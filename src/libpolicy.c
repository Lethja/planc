#include "config.h"
#include "libdomain.h"
#include "libdatabase.h"
#include <stdlib.h>
#include <string.h>
#include <webkit2/webkit-web-extension.h>

static gboolean
c_webpage_send_request(WebKitWebPage *wp, WebKitURIRequest *req, WebKitURIResponse *res, gpointer user_data) {
    if (res)
        return FALSE;
    const gchar *purl = webkit_web_page_get_uri(wp);
    const gchar *rurl = webkit_uri_request_get_uri(req);
    gchar *pdom = getDomainName(purl);
    gchar *rdom = getDomainName(rurl);
    if (pdom && rdom) {
        gboolean r;
        if (strcmp(pdom, rdom) == 0)
            r = FALSE;
        else {
            if (sql_domain_policy_read(pdom, rdom)) {
#ifndef NDEBUG
                g_debug("PASS: '%s' > '%s'", pdom, rdom);
#endif
                r = FALSE;
            } else {
#ifndef NDEBUG
                g_debug("DENY: '%s' > '%s'", pdom, rdom);
#endif
                r = TRUE;
            }
        }
        free(pdom);
        free(rdom);
        return r;
    }
    if (pdom)
        free(pdom);
    if (rdom)
        free(rdom);
    return FALSE;
}

static gboolean c_webpage_context(WebKitWebPage *wp, WebKitContextMenu *cm, WebKitWebHitTestResult *hr, void *v) {
    g_autofree char *string = NULL;
    GVariantBuilder builder;
    WebKitFrame *frame;
    g_autoptr(JSCContext)
    js_context = NULL;
    g_autoptr(JSCValue)
    js_value = NULL;

    /* https://gitlab.gnome.org/GNOME/epiphany/issues/442 */
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    if (!(frame = webkit_web_page_get_main_frame(wp)))
        return FALSE;
    G_GNUC_END_IGNORE_DEPRECATIONS

    if (!(js_context = webkit_frame_get_js_context(frame)))
        return FALSE;

    js_value = jsc_context_evaluate(js_context, "window.getSelection().toString();", -1);
    if (!jsc_value_is_null(js_value) && !jsc_value_is_undefined(js_value))
        string = jsc_value_to_string(js_value);

    if (!string || *string == '\0')
        return FALSE;

    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&builder, "{sv}", "SelectedText", g_variant_new_string(g_strstrip(string)));
    webkit_context_menu_set_user_data(cm, g_variant_builder_end(&builder));
#ifndef NDEBUG
    g_debug("Page selection is %s", string);
#endif
    return TRUE;
}

static void web_page_created_callback(WebKitWebExtension *extension, WebKitWebPage *wp, gpointer user_data) {
    g_signal_connect(wp, "send-request", G_CALLBACK(c_webpage_send_request), NULL);
    g_signal_connect(wp, "context-menu", G_CALLBACK(c_webpage_context), NULL);
}

G_MODULE_EXPORT void webkit_web_extension_initialize(WebKitWebExtension *extension) {
    g_signal_connect(extension, "page-created", G_CALLBACK(web_page_created_callback), NULL);
}
