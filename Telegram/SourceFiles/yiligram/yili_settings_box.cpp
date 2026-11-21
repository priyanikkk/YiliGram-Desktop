/*
This file is part of YiliGram Desktop,
an unofficial Telegram client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/priyanikkk/YiliGram-Desktop/blob/main/LEGAL
*/
#include "yiligram/yili_settings_box.h"

#include "lang/lang_keys.h"
#include "ui/layers/generic_box.h"
#include "ui/painter.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "yiligram/yili_settings.h"
#include "yiligram/yili_shapes.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"

namespace Yili {
namespace {

constexpr auto kColumns = 5;

void AddFramePicker(not_null<Ui::VerticalLayout*> container) {
	const auto picker = container->add(
		object_ptr<Ui::RpWidget>(container),
		st::boxRowPadding);
	picker->setCursor(style::cur_pointer);

	const auto rows = (kAvatarFrameCount + kColumns - 1) / kColumns;
	const auto cell = [=] {
		return picker->width() / kColumns;
	};
	const auto tileRect = [=](int index) {
		const auto size = cell();
		return QRect(
			(index % kColumns) * size,
			(index / kColumns) * size,
			size,
			size);
	};
	picker->widthValue() | rpl::on_next([=](int width) {
		picker->resize(width, (width / kColumns) * rows);
	}, picker->lifetime());

	picker->paintRequest() | rpl::on_next([=] {
		auto p = QPainter(picker);
		auto hq = PainterHighQualityEnabler(p);
		const auto current = int(CurrentAvatarFrame());
		const auto size = cell();
		const auto padding = size * 0.16;
		for (auto i = 0; i != kAvatarFrameCount; ++i) {
			const auto rect = QRectF(tileRect(i));
			if (i == current) {
				p.setPen(Qt::NoPen);
				p.setBrush(st::windowBgOver);
				const auto radius = size * 0.22;
				p.drawRoundedRect(
					rect.marginsRemoved({ 2., 2., 2., 2. }),
					radius,
					radius);
			}
			const auto inner = rect.marginsRemoved(
				{ padding, padding, padding, padding });
			auto gradient = QLinearGradient(
				inner.topLeft(),
				inner.bottomRight());
			gradient.setStops({
				{ 0., st::windowBgActive->c },
				{ 1., st::activeButtonBg->c },
			});
			p.setPen(Qt::NoPen);
			p.setBrush(gradient);
			p.drawPath(AvatarFramePath(AvatarFrame(i), inner));
		}
	}, picker->lifetime());

	picker->events() | rpl::filter([](not_null<QEvent*> e) {
		return e->type() == QEvent::MouseButtonRelease;
	}) | rpl::on_next([=](not_null<QEvent*> e) {
		const auto position = static_cast<QMouseEvent*>(e.get())->pos();
		for (auto i = 0; i != kAvatarFrameCount; ++i) {
			if (tileRect(i).contains(position)) {
				SaveAvatarFrame(AvatarFrame(i));
				picker->update();
				break;
			}
		}
	}, picker->lifetime());
}

void AddToggle(
		not_null<Ui::VerticalLayout*> container,
		QString text,
		bool value,
		Fn<void(bool)> callback) {
	container->add(object_ptr<Ui::SettingsButton>(
		container,
		rpl::single(std::move(text)),
		st::settingsButtonNoIcon
	))->toggleOn(
		rpl::single(value)
	)->toggledChanges(
	) | rpl::on_next(std::move(callback), container->lifetime());
}

} // namespace

void SettingsBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(rpl::single(u"YiliGram"_q));
	box->setWidth(st::boxWideWidth);

	const auto container = box->verticalLayout();

	Ui::AddSubsectionTitle(
		container,
		rpl::single(Tr("Avatar shape", "Форма аватарок", "Форма аватарок")));
	AddFramePicker(container);
	Ui::AddSkip(container);
	Ui::AddDivider(container);
	Ui::AddSkip(container);

	Ui::AddSubsectionTitle(
		container,
		rpl::single(Tr("Chats", "Чаты", "Чати")));
	AddToggle(
		container,
		Tr("Bubble tails", "Хвостики у сообщений", "Хвостики повідомлень"),
		BubbleTails(),
		SetBubbleTails);
	AddToggle(
		container,
		Tr(
			"Call button in the chat header",
			"Кнопка звонка в шапке чата",
			"Кнопка дзвінка в шапці чату"),
		HeaderCallButton(),
		SetHeaderCallButton);
	AddToggle(
		container,
		Tr(
			"Hide the gift button",
			"Скрыть кнопку подарка",
			"Сховати кнопку подарунка"),
		HideGiftButton(),
		SetHideGiftButton);
	Ui::AddSkip(container);
	Ui::AddDividerText(
		container,
		rpl::single(Tr(
			"YiliGram badges are granted from a profile: "
			"the ⋮ menu, then \"YiliGram badge\".",
			"Бейджи YiliGram выдаются в профиле: "
			"меню ⋮, затем «Бейдж YiliGram».",
			"Бейджі YiliGram видаються в профілі: "
			"меню ⋮, потім «Бейдж YiliGram».")));

	box->addButton(tr::lng_close(), [=] { box->closeBox(); });
}

} // namespace Yili
