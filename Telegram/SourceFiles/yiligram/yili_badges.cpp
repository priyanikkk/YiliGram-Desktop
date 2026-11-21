/*
This file is part of YiliGram Desktop,
an unofficial Telegram client based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/priyanikkk/YiliGram-Desktop/blob/main/LEGAL
*/
#include "yiligram/yili_badges.h"

#include "api/api_common.h"
#include "apiwrap.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "history/history.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "ui/abstract_button.h"
#include "ui/layers/generic_box.h"
#include "ui/painter.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "window/themes/window_theme.h"
#include "window/window_session_controller.h"
#include "yiligram/yili_settings.h"
#include "yiligram/yili_shapes.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtWidgets/QApplication>

namespace Yili {
namespace {

constexpr auto kTag = "#yiligram_badges";
constexpr auto kRefreshTimeout = 30 * 60 * crl::time(1000);

struct Colors {
	QColor primary;
	QColor onPrimary;
	QColor secondary;
	QColor tertiary;
	QColor tertiaryContainer;
	QColor onTertiaryContainer;
};

// Material 3 tones of the YiliGram seed color #8C7A5B.
[[nodiscard]] Colors CurrentColors() {
	if (Window::Theme::IsNightMode()) {
		return {
			QColor(0xec, 0xc0, 0x6c),
			QColor(0x41, 0x2d, 0x00),
			QColor(0xd9, 0xc4, 0xa0),
			QColor(0xb2, 0xcf, 0xa7),
			QColor(0x35, 0x4d, 0x2f),
			QColor(0xce, 0xeb, 0xc2),
		};
	}
	return {
		QColor(0x7a, 0x59, 0x0c),
		QColor(0xff, 0xff, 0xff),
		QColor(0x6c, 0x5c, 0x3f),
		QColor(0x4c, 0x65, 0x45),
		QColor(0xce, 0xeb, 0xc2),
		QColor(0x0a, 0x20, 0x07),
	};
}

base::flat_map<int64, BadgeType> Badges;
bool Loaded = false;
base::flat_map<uint64, crl::time> LastFetch;
rpl::event_stream<> Changes;

void Notify() {
	Changes.fire({});
	for (const auto widget : QApplication::topLevelWidgets()) {
		widget->update();
	}
}

[[nodiscard]] QString TypeKey(BadgeType type) {
	switch (type) {
	case BadgeType::Admin: return u"admin"_q;
	case BadgeType::Beta: return u"beta"_q;
	case BadgeType::Official: return u"official"_q;
	}
	Unexpected("Type in Yili::TypeKey.");
}

constexpr auto kTypes = std::array{
	BadgeType::Admin,
	BadgeType::Beta,
	BadgeType::Official,
};

void Parse(const QByteArray &json) {
	Badges.clear();
	const auto object = QJsonDocument::fromJson(json).object();
	for (const auto type : kTypes) {
		for (const auto value : object.value(TypeKey(type)).toArray()) {
			const auto id = value.isString()
				? value.toString().toLongLong()
				: int64(value.toDouble());
			if (id) {
				Badges[id] = type;
			}
		}
	}
}

[[nodiscard]] QByteArray Serialize() {
	auto object = QJsonObject();
	for (const auto type : kTypes) {
		auto list = QJsonArray();
		for (const auto &[id, badge] : Badges) {
			if (badge == type) {
				list.push_back(double(id));
			}
		}
		object.insert(TypeKey(type), list);
	}
	return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

void EnsureLoaded() {
	if (!Loaded) {
		Loaded = true;
		Parse(BadgesJson());
	}
}

[[nodiscard]] int64 DialogId(not_null<PeerData*> peer) {
	if (peer->isUser()) {
		return int64(peerToUser(peer->id).bare);
	} else if (peer->isChat()) {
		return -int64(peerToChat(peer->id).bare);
	} else if (peer->isChannel()) {
		return -int64(peerToChannel(peer->id).bare);
	}
	return 0;
}

void Apply(const QString &text) {
	const auto json = text.mid(QString::fromLatin1(kTag).size()).trimmed();
	const auto was = Serialize();
	Parse(json.toUtf8());
	const auto now = Serialize();
	if (was != now) {
		SetBadgesJson(now);
		Notify();
	}
}

void Refresh(not_null<Main::Session*> session) {
	const auto key = session->uniqueId();
	const auto now = crl::now();
	const auto i = LastFetch.find(key);
	if (i != end(LastFetch) && now - i->second < kRefreshTimeout) {
		return;
	}
	LastFetch[key] = now;
	const auto self = session->user();
	session->api().request(MTPmessages_Search(
		MTP_flags(0),
		self->input(),
		MTP_string(kTag),
		MTP_inputPeerEmpty(),
		MTP_inputPeerEmpty(),
		MTP_vector<MTPReaction>(),
		MTP_int(0), // top_msg_id
		MTP_inputMessagesFilterEmpty(),
		MTP_int(0), // min_date
		MTP_int(0), // max_date
		MTP_int(0), // offset_id
		MTP_int(0), // add_offset
		MTP_int(5), // limit
		MTP_int(0), // max_id
		MTP_int(0), // min_id
		MTP_long(0) // hash
	)).done([=](const MTPmessages_Messages &result) {
		auto newestDate = 0;
		auto newest = QString();
		const auto handle = [&](const QVector<MTPMessage> &list) {
			for (const auto &message : list) {
				message.match([&](const MTPDmessage &data) {
					const auto text = qs(data.vmessage());
					if (text.startsWith(QString::fromLatin1(kTag))
						&& data.vdate().v > newestDate) {
						newestDate = data.vdate().v;
						newest = text;
					}
				}, [](const auto &) {});
			}
		};
		result.match([&](const MTPDmessages_messagesNotModified &) {
		}, [&](const auto &data) {
			handle(data.vmessages().v);
		});
		if (!newest.isEmpty()) {
			EnsureLoaded();
			Apply(newest);
		}
	}).send();
}

void Grant(
		not_null<Main::Session*> session,
		int64 id,
		std::optional<BadgeType> type) {
	EnsureLoaded();
	if (type) {
		Badges[id] = *type;
	} else {
		Badges.remove(id);
	}
	const auto json = Serialize();
	SetBadgesJson(json);
	Notify();

	const auto history = session->data().history(session->user());
	auto message = Api::MessageToSend(Api::SendAction(history));
	message.textWithTags = TextWithTags{
		QString::fromLatin1(kTag) + '\n' + QString::fromUtf8(json),
	};
	session->api().sendMessage(std::move(message));
}

[[nodiscard]] QString Title(std::optional<BadgeType> type) {
	if (!type) {
		return Tr("No badge", "Без бейджа", "Без бейджа");
	}
	switch (*type) {
	case BadgeType::Admin:
		return Tr("YiliGram team", "Команда YiliGram", "Команда YiliGram");
	case BadgeType::Beta:
		return Tr(
			"YiliGram beta tester",
			"Бета-тестер YiliGram",
			"Бета-тестер YiliGram");
	case BadgeType::Official:
		return Tr(
			"Official YiliGram resource",
			"Официальный ресурс YiliGram",
			"Офіційний ресурс YiliGram");
	}
	Unexpected("Type in Yili::Title.");
}

[[nodiscard]] QString About(BadgeType type) {
	switch (type) {
	case BadgeType::Admin:
		return Tr(
			"Member of the YiliGram team: makes the app, answers "
			"questions and hands out badges.",
			"Участник команды YiliGram: делает приложение, отвечает "
			"на вопросы и выдаёт бейджи.",
			"Учасник команди YiliGram: робить застосунок, відповідає "
			"на питання та видає бейджі.");
	case BadgeType::Beta:
		return Tr(
			"YiliGram beta tester: tries new versions before everyone "
			"else and helps find bugs.",
			"Бета-тестер YiliGram: первым пробует новые версии "
			"и помогает находить ошибки.",
			"Бета-тестер YiliGram: першим пробує нові версії "
			"та допомагає знаходити помилки.");
	case BadgeType::Official:
		return Tr(
			"An official YiliGram channel or group: news, updates and "
			"support straight from the YiliGram team.",
			"Официальный канал или группа YiliGram: новости, обновления "
			"и поддержка напрямую от команды YiliGram.",
			"Офіційний канал або група YiliGram: новини, оновлення "
			"та підтримка напряму від команди YiliGram.");
	}
	Unexpected("Type in Yili::About.");
}

void BadgeInfoBox(not_null<Ui::GenericBox*> box, BadgeType type) {
	box->setWidth(st::boxWideWidth);

	const auto size = st::boxTitleHeight + st::boxPadding.top() * 2;
	auto owned = object_ptr<Ui::RpWidget>(box);
	owned->resize(size, size);
	owned->setNaturalWidth(size);
	const auto icon = box->addRow(
		std::move(owned),
		st::boxRowPadding + QMargins(0, st::boxPadding.top(), 0, 0),
		style::al_top);
	icon->paintRequest() | rpl::on_next([=] {
		auto p = QPainter(icon);
		PaintBadge(p, type, QRectF(icon->rect()));
	}, icon->lifetime());

	box->addRow(
		object_ptr<Ui::FlatLabel>(box, Title(type), st::boxTitle),
		st::boxRowPadding + QMargins(0, st::boxPadding.top(), 0, 0),
		style::al_top);
	box->addRow(
		object_ptr<Ui::FlatLabel>(
			box,
			About(type),
			st::changePhoneDescription),
		st::boxRowPadding + st::changePhoneDescriptionPadding);
	box->addButton(tr::lng_close(), [=] { box->closeBox(); });
}

void BadgeGrantBox(
		not_null<Ui::GenericBox*> box,
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	box->setTitle(rpl::single(
		Tr("YiliGram badge", "Бейдж YiliGram", "Бейдж YiliGram")));
	box->setWidth(st::boxWideWidth);

	const auto id = DialogId(peer);
	auto options = std::vector<std::optional<BadgeType>>{ std::nullopt };
	if (peer->isUser()) {
		options.push_back(BadgeType::Admin);
		options.push_back(BadgeType::Beta);
	} else {
		options.push_back(BadgeType::Official);
	}
	const auto current = BadgeFor(peer);
	const auto session = &peer->session();
	for (const auto &option : options) {
		const auto button = box->addRow(
			object_ptr<Ui::SettingsButton>(
				box,
				rpl::single(Title(option)
					+ ((option == current) ? u" ✓"_q : QString())),
				st::settingsButtonNoIcon),
			QMargins());
		button->setClickedCallback([=] {
			if (option != current) {
				Grant(session, id, option);
				controller->showToast(option
					? Tr(
						"Badge granted. The list was posted to Saved Messages.",
						"Бейдж выдан. Список отправлен в «Избранное».",
						"Бейдж видано. Список надіслано в «Збережене».")
					: Tr(
						"Badge removed. The list was posted to Saved Messages.",
						"Бейдж снят. Список отправлен в «Избранное».",
						"Бейдж знято. Список надіслано в «Збережене»."));
			}
			box->closeBox();
		});
	}
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

} // namespace

std::optional<BadgeType> BadgeFor(not_null<PeerData*> peer) {
	EnsureLoaded();
	Refresh(&peer->session());
	const auto i = Badges.find(DialogId(peer));
	return (i != end(Badges)) ? std::make_optional(i->second) : std::nullopt;
}

rpl::producer<> BadgesChanges() {
	return Changes.events();
}

void PaintBadge(QPainter &p, BadgeType type, const QRectF &rect) {
	const auto colors = CurrentColors();
	auto hq = PainterHighQualityEnabler(p);
	p.setPen(Qt::NoPen);
	const auto cx = rect.center().x();
	const auto cy = rect.center().y();
	const auto r = std::min(rect.width(), rect.height()) / 2.;
	switch (type) {
	case BadgeType::Admin: {
		// Soft burst, primary -> tertiary, with a four-point sparkle.
		auto gradient = QLinearGradient(rect.topLeft(), rect.bottomRight());
		gradient.setStops({
			{ 0., colors.primary },
			{ 1., colors.tertiary },
		});
		p.setBrush(gradient);
		p.drawPath(ShapePath(Shape::SoftBurst, rect));
		const auto o = r * 0.52;
		const auto i = r * 0.13;
		auto sparkle = QPainterPath();
		sparkle.moveTo(cx, cy - o);
		sparkle.quadTo(cx + i, cy - i, cx + o, cy);
		sparkle.quadTo(cx + i, cy + i, cx, cy + o);
		sparkle.quadTo(cx - i, cy + i, cx - o, cy);
		sparkle.quadTo(cx - i, cy - i, cx, cy - o);
		sparkle.closeSubpath();
		p.setBrush(colors.onPrimary);
		p.drawPath(sparkle);
	} break;
	case BadgeType::Official: {
		// Sunny, secondary -> primary, with a check mark.
		auto gradient = QLinearGradient(rect.bottomLeft(), rect.topRight());
		gradient.setStops({
			{ 0., colors.secondary },
			{ 1., colors.primary },
		});
		p.setBrush(gradient);
		p.drawPath(ShapePath(Shape::Sunny, rect));
		auto pen = QPen(colors.onPrimary);
		pen.setWidthF(r * 0.2);
		pen.setCapStyle(Qt::RoundCap);
		pen.setJoinStyle(Qt::RoundJoin);
		p.setPen(pen);
		p.setBrush(Qt::NoBrush);
		auto check = QPainterPath();
		check.moveTo(cx - r * 0.38, cy + r * 0.02);
		check.lineTo(cx - r * 0.1, cy + r * 0.3);
		check.lineTo(cx + r * 0.4, cy - r * 0.26);
		p.drawPath(check);
	} break;
	case BadgeType::Beta: {
		// Four-lobe cookie with a bold beta.
		p.setBrush(colors.tertiaryContainer);
		p.drawPath(ShapePath(Shape::Cookie4, rect));
		auto font = st::semiboldFont->f;
		font.setBold(true);
		font.setPixelSize(std::max(int(r * 1.25), 1));
		p.setFont(font);
		p.setPen(colors.onTertiaryContainer);
		p.drawText(rect, Qt::AlignCenter, QString(QChar(0x03B2)));
	} break;
	}
}

void ShowBadgeInfo(
		not_null<Window::SessionController*> controller,
		BadgeType type) {
	controller->show(Box(BadgeInfoBox, type));
}

void ShowBadgeGrant(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	controller->show(Box(BadgeGrantBox, controller, peer));
}

not_null<Ui::RpWidget*> CreateBadgeButton(
		not_null<QWidget*> parent,
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer,
		int size) {
	const auto button = Ui::CreateChild<Ui::AbstractButton>(parent.get());
	button->resize(size, size);
	const auto refresh = [=] {
		button->setVisible(BadgeFor(peer).has_value());
		button->update();
	};
	refresh();
	BadgesChanges() | rpl::on_next(refresh, button->lifetime());
	button->paintRequest() | rpl::on_next([=] {
		if (const auto type = BadgeFor(peer)) {
			auto p = QPainter(button);
			const auto inner = QRectF(button->rect()).marginsRemoved(
				QMarginsF(size * 0.06, size * 0.06, size * 0.06, size * 0.06));
			PaintBadge(p, *type, inner);
		}
	}, button->lifetime());
	button->setClickedCallback([=] {
		if (const auto type = BadgeFor(peer)) {
			ShowBadgeInfo(controller, *type);
		}
	});
	return button;
}

} // namespace Yili
