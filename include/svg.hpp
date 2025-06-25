#pragma once

#include <QIcon>
#include <QSvgRenderer>
#include <QPainter>

template <typename T, std::size_t U>
QIcon icon_from_svg(const std::array<T, U>& data, const QSize& size) {
	const char* raw = reinterpret_cast<const char*>(data.data());
	QByteArray bytes(raw, static_cast<int>(U * sizeof(T)));

	QSvgRenderer svgRenderer(bytes);
	if (!svgRenderer.isValid()) {
		return {};
	}

	QPixmap pixmap(size);
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);
	svgRenderer.render(&painter);
	painter.end();

	return {pixmap};
}