/*
This file is part of YiliGram Desktop,
an unofficial Telegram client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/priyanikkk/YiliGram-Desktop/blob/main/LEGAL
*/
#pragma once

#include <QtGui/QPainterPath>
#include <QtGui/QImage>

namespace Yili {

// Material 3 Expressive shapes, the same set as YiliGram for Android.
enum class Shape : uchar {
	Cookie4,
	Cookie9,
	Cookie12,
	Clover4,
	Sunny,
	SoftBurst,
	Flower,
	Pentagon,
	Gem,
	Square,
};

[[nodiscard]] QPainterPath ShapePath(Shape shape, const QRectF &rect);

enum class AvatarFrame : uchar {
	Circle,
	Cookie9,
	Cookie12,
	Clover4,
	Sunny,
	SoftBurst,
	Flower,
	Pentagon,
	Gem,
	Square,
};
inline constexpr auto kAvatarFrameCount = 10;

[[nodiscard]] AvatarFrame CurrentAvatarFrame();
void SetAvatarFrame(AvatarFrame frame);
[[nodiscard]] int AvatarFrameVersion();

[[nodiscard]] QPainterPath AvatarFramePath(AvatarFrame frame, const QRectF &rect);
[[nodiscard]] QPainterPath AvatarFramePath(const QRectF &rect);

// Replaces Images::Circle() for userpics: cuts the image to the current frame.
[[nodiscard]] QImage ApplyAvatarFrame(QImage image);

} // namespace Yili
