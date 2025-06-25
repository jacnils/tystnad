#pragma once

#include <QSettings>

template<typename T>
void save_setting(const QString& key, const T& value) {
	QSettings settings("Jacob Nilsson", "tystnad");
	settings.setValue(key, value);
#if TYSTNAD_DEBUG
	std::cerr << "Value '" << key.toStdString() << "' set to '" << value << "'\n";
#endif
}

void save_setting(const QString& key, const std::string& value) {
	QSettings settings("Jacob Nilsson", "tystnad");
	settings.setValue(key, QString::fromStdString(value));
#if TYSTNAD_DEBUG
	std::cerr << "Value '" << key.toStdString() << "' set to '" << value << "'\n";
#endif
}

template<typename T>
T load_setting(const QString& key, const T& value = T()) {
    QSettings settings("Jacob Nilsson", "tystnad");
    QVariant v = settings.value(key);
    if (!v.isValid()) {
        #if TYSTNAD_DEBUG
        std::cerr << "Value '" << key.toStdString() << "' not found. Returning default.\n";
        #endif
        return value;
    }
    #if TYSTNAD_DEBUG
    std::cerr << "Value '" << key.toStdString() << "' is now '" << v.value<T>() << "'\n";
    #endif
    return v.value<T>();
}

std::string load_setting(const QString& key, const std::string& value = std::string()) {
    QSettings settings("Jacob Nilsson", "tystnad");
    QVariant v = settings.value(key);
    if (!v.isValid()) {
        #if TYSTNAD_DEBUG
        std::cerr << "Value '" << key.toStdString() << "' not found. Returning default.\n";
        #endif
        return value;
    }
    #if TYSTNAD_DEBUG
    std::cerr << "Value '" << key.toStdString() << "' is now '" << v.toString().toStdString() << "'\n";
    #endif
    QString qstr = v.toString();
    if (qstr.isEmpty()) return value;
    return qstr.toStdString();
}
