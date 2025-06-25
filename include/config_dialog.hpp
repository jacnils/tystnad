#pragma once

#include <QDialog>
#include <QFileDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

class config_dialog : public QDialog {
	Q_OBJECT

public:
	~config_dialog() override;
	config_dialog(int,
#ifdef MACOS
		bool,
#endif
		std::string,
#ifdef LINUX
		std::string,
#endif
		QWidget* = nullptr);

	int audio_length() const;
	std::string custom_audio_file() const;
#ifdef LINUX
	std::string get_alsa_sink() const;
#endif
#ifdef MACOS
	bool run_on_startup() const;
#endif

private:
	QSpinBox* length_box;
	QCheckBox* startup_box;
	QPushButton* ok_button;
	QPushButton* cancel_button;
	QLabel* audio_input_label;
	QLineEdit* audio_input;
	QLabel* alsa_sink_label;
	QLineEdit* alsa_sink;
	QPushButton* browse_button;
};
