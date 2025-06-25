#include <config_dialog.hpp>
#include <string>
#include <QSpinBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>

config_dialog::config_dialog(int length,
#ifdef MACOS
    bool run_on_startup,
#endif
    std::string file,
#ifdef LINUX
    std::string sink,
#endif
    QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");

    length_box = new QSpinBox(this);
    length_box->setMinimum(1);
    length_box->setMaximum(3600);
    length_box->setValue(length);
    length_box->setSuffix(" sec");

#if MACOS
    startup_box = new QCheckBox("Run on startup", this);
    startup_box->setChecked(run_on_startup);
#endif

    audio_input_label = new QLabel("Audio Input File:", this);
    audio_input = new QLineEdit(this);
    audio_input->setText(QString::fromStdString(file));
    browse_button = new QPushButton("Browse...", this);
    connect(browse_button, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "Select Audio File", QString(), "Audio Files (*.wav *.mp3 *.flac);;All Files (*)");
        if (!file.isEmpty()) {
            audio_input->setText(file);
        }
    });

    ok_button = new QPushButton("OK", this);
    cancel_button = new QPushButton("Cancel", this);

    connect(ok_button, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);

    QVBoxLayout* main_layout = new QVBoxLayout(this);

    QHBoxLayout* length_layout = new QHBoxLayout();
    length_layout->addWidget(new QLabel("Audio Length:", this));
    length_layout->addWidget(length_box);
    main_layout->addLayout(length_layout);

#ifdef MACOS
    main_layout->addWidget(startup_box);
#endif

    QHBoxLayout* audio_input_layout = new QHBoxLayout();
    audio_input_layout->addWidget(audio_input_label);
    audio_input_layout->addWidget(audio_input);
    audio_input_layout->addWidget(browse_button);

    main_layout->addLayout(audio_input_layout);

#ifdef LINUX
    alsa_sink_label = new QLabel("ALSA sink:", this);
    alsa_sink = new QLineEdit(this);
    alsa_sink->setText(QString::fromStdString(sink));

    QHBoxLayout* alsa_sink_layout = new QHBoxLayout();

    alsa_sink_layout->addWidget(alsa_sink_label);
    alsa_sink_layout->addWidget(alsa_sink);

    main_layout->addLayout(alsa_sink_layout);
#endif

    QHBoxLayout* button_layout = new QHBoxLayout();
    button_layout->addStretch();
    button_layout->addWidget(ok_button);
    button_layout->addWidget(cancel_button);

    main_layout->addLayout(button_layout);
}

std::string config_dialog::custom_audio_file() const {
    return this->audio_input->text().toStdString();
}

int config_dialog::audio_length() const {
	return this->length_box->value();
}

#ifdef MACOS
bool config_dialog::run_on_startup() const {
	return this->startup_box->isChecked();
}
#endif
#ifdef LINUX
std::string config_dialog::get_alsa_sink() const {
	return this->alsa_sink->text().toStdString();
}
#endif

config_dialog::~config_dialog() = default;
