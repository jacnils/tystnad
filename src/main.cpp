#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtSvgWidgets/qsvgwidget.h>

#include <thread>
#include <iostream>

#include <logo.svg.hpp>
#include <logo-off.svg.hpp>
#include <logo-on.svg.hpp>

#include <config_dialog.hpp>
#include <audio_manager.hpp>
#if MACOS
#include <launch_agent.hpp>
#endif
#include <setting.hpp>
#include <svg.hpp>
#include <wav.hpp>

#include <fstream>
#include <string>
#include <filesystem>
#include <unistd.h>
#include <pwd.h>

std::atomic<int> length{500};
#if MACOS
std::atomic<bool> run_on_startup{false};
#endif
std::atomic<bool> state{false};
std::string custom_audio_file{};
std::string alsa_sink{"default"};

int main(int argc, char *argv[]) {
	QApplication::setQuitOnLastWindowClosed(false);

	QApplication app(argc, argv);

	if (!QSystemTrayIcon::isSystemTrayAvailable()) {
		qCritical() << "System tray is not available!";
		return 1;
	}

	auto tray_icon = std::make_shared<QSystemTrayIcon>();
	auto tray = std::make_shared<QMenu>();

	state = load_setting<bool>("state");
	length = load_setting<int>("audio_length");
	custom_audio_file = load_setting("custom_audio_file", custom_audio_file);
	alsa_sink = load_setting("alsa_sink", alsa_sink);
#if MACOS
	run_on_startup = load_setting<bool>("run_on_startup");
#endif

	auto toggle_action = std::make_shared<QAction>(state ? "Turn Off" : "Turn On");
	auto quit_action = std::make_shared<QAction>("Quit");
	auto about_action = std::make_shared<QAction>("About");
	auto configure_action = std::make_shared<QAction>("Configure");

	tray_icon->setIcon(icon_from_svg(state ? logo_on_svg : logo_off_svg, QSize(64, 32)));
	tray_icon->setToolTip("tystnad");
	tray_icon->setContextMenu(tray.get());
	tray_icon->show();

	QObject::connect(toggle_action.get(), &QAction::triggered, [=]() mutable {
		state = !state;

		save_setting("state", state.load());

		toggle_action->setText(state ? "Turn Off" : "Turn On");

		QIcon icon = icon_from_svg(state ? logo_on_svg : logo_off_svg, QSize(64, 32));
		tray_icon->setIcon(icon);
	});
	QObject::connect(about_action.get(), &QAction::triggered, [=]() mutable {
		QDialog dialog;
		dialog.setWindowTitle("About tystnad");
		QVBoxLayout layout(&dialog);

		QByteArray bytes(reinterpret_cast<const char*>(logo_svg.data()), static_cast<int>(logo_svg.size()));

		auto widget = std::make_shared<QSvgWidget>();

		widget->load(bytes);
		widget->setFixedSize(300, 150);

		layout.addWidget(widget.get());

		QLabel label;
		label.setTextFormat(Qt::RichText);
		label.setTextInteractionFlags(Qt::TextBrowserInteraction);
		label.setOpenExternalLinks(true);

		label.setText(
			("<h2>tystnad "
#ifdef TYSTNAD_VERSION
			 + std::string(TYSTNAD_VERSION) + "</h2><br>"
#else
			"</h2><br>"
#endif
			 "<small>Written by Jacob Nilsson.</small><br>"
			 "<small>Copyright (c) Jacob Nilsson</small><br>"
			 "<small>Licensed under the MIT license.</small><br><br>"
			 "<small><a href=\"https://github.com/jacnils/tystnad\">GitHub Repository</a></small>").data()
		);

		layout.addWidget(&label);

		QPushButton ok_button("OK");
		QObject::connect(&ok_button, &QPushButton::clicked, &dialog, [&]() mutable {
			dialog.accept();
		});
		layout.addWidget(&ok_button);

		dialog.exec();
	});
	QObject::connect(configure_action.get(), &QAction::triggered, [=]() mutable {
	#if MACOS
		auto* dialog = new config_dialog(length.load(), run_on_startup.load(), custom_audio_file, nullptr);
	#else
		auto* dialog = new config_dialog(length.load(), custom_audio_file, alsa_sink, nullptr);
	#endif

		QObject::connect(dialog, &QDialog::accepted, [=]() {
			// get from the dialog and immediately save
			length = dialog->audio_length();
			custom_audio_file = dialog->custom_audio_file();
#if LINUX
			alsa_sink = dialog->get_alsa_sink();
#endif
	#if MACOS
			run_on_startup = dialog->run_on_startup();
	#endif

			save_setting("audio_length", length.load());
			save_setting("custom_audio_file", custom_audio_file);
#if LINUX
			save_setting("alsa_sink", alsa_sink.empty() ? "default" : alsa_sink);
#endif
	#if MACOS
			save_setting("run_on_startup", run_on_startup.load());

			if (run_on_startup) {
				write_launch_agent(get_executable_path());
			} else {
				remove_launch_agent();
			}
	#endif
			dialog->deleteLater();
		});

		QObject::connect(dialog, &QDialog::rejected, dialog, &QObject::deleteLater);

		dialog->setAttribute(Qt::WA_DeleteOnClose);
		dialog->show();
	});

	QObject::connect(quit_action.get(), &QAction::triggered, &app, &QApplication::quit);

	tray->addAction(toggle_action.get());
	tray->addSeparator();
	tray->addAction(quit_action.get());
	tray->addSeparator();
	tray->addAction(about_action.get());
	tray->addAction(configure_action.get());

	int initial_length = length;
	auto silent = generate_empty_sound(length);

	std::atomic<bool> applied = false;

	// hacky but works?
	std::thread t([&]() {
		while (true) { //NOLINT
			if (state.load(std::memory_order_acquire)) {
				audio_manager p;

				if (!applied && custom_audio_file.empty()) {
					applied = true;

					static constexpr int duration = 50;
					static constexpr int rate = 44100;
					static constexpr int channels = 2;
					static constexpr int fade_in = (duration * rate) / 1000;

					apply_fade_in(silent, fade_in, channels);
				}

				try {
					if (!custom_audio_file.empty()) {
						if (!p.init(custom_audio_file
#if LINUX
									, alsa_sink.empty() ? "default" : alsa_sink
#endif
							   )) {
							throw std::runtime_error{"Failed to play audio"};
						}
					} else {
						if (!p.init(silent
#if LINUX
									, alsa_sink.empty() ? "default" : alsa_sink
#endif
							   )) {
							throw std::runtime_error{"Failed to play audio"};
						}
					}

					p.wait_until_done();
				} catch (std::exception& e) {
					QMessageBox::critical(nullptr, "Error", QString("An error occurred:\n%1").arg(e.what()));
					state = false;

				}

				if (initial_length != length && custom_audio_file.empty()) {
					silent = generate_empty_sound(length);
					applied = false;
				}

				initial_length = length;
			} else {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
	}); //NOLINT

	t.detach();

	return QApplication::exec();
}
