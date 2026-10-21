/*
 * 暗黙の修飾キー(LS(MINUS)のShiftなど)を、キー本体より先に別のHIDレポートで送るリスナー
 *
 * ZMKは修飾キーとキー本体を同じレポートで送る。Windowsのリモートデスクトップは
 * このShiftを取りこぼすため、Layout Shift(JIS)で`=`がLS(MINUS)に変換されると`-`になる。
 * 修飾キーだけのレポートを先に送り、キー本体は後続のhid_listenerに送らせる。
 *
 * 注意: hid_listenerより先に呼ばれる必要がある。リスナーの呼び出し順はリンク順で決まるため、
 * CMakeLists.txtでtarget_sources(app ...)として追加すること(ZMK本体のソースより前に並ぶ)
 */

#include <zephyr/logging/log.h>

#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/hid_usage_pages.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int implicit_mods_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL || !ev->state || ev->implicit_modifiers == 0) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // 修飾キーが変化した場合だけ、キー本体を含まないレポートを先に送る
    if (zmk_hid_implicit_modifiers_press(ev->implicit_modifiers) > 0) {
        int err = zmk_endpoints_send_report(HID_USAGE_KEY);
        if (err < 0) {
            LOG_ERR("Failed to send implicit modifiers report (%d)", err);
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(implicit_mods_listener, implicit_mods_listener);
ZMK_SUBSCRIPTION(implicit_mods_listener, zmk_keycode_state_changed);
