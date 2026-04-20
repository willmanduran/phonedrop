#ifndef I18N_H
#define I18N_H

#include <string.h>
#include <stdlib.h>

typedef struct {
    const char *title;
    const char *exit_btn;
    const char *donate_text;
    const char *dl_prefix;
    const char *shutdown_msg;
    const char *gui_drag_label;
    const char *gui_add_btn;
    const char *gui_gen_qr_btn;
    const char *gui_files_ready;
} i18n_bundle;

static i18n_bundle lang_es = {
    .title = "SERVIDOR LISTO",
    .exit_btn = "TERMINAR SESIÓN",
    .donate_text = "Apoya este proyecto",
    .dl_prefix = "Descargar",
    .shutdown_msg = "SESIÓN TERMINADA",
    .gui_drag_label = "ARRASTRA LOS ARCHIVOS AQUÍ",
    .gui_add_btn = "AÑADIR ARCHIVOS",
    .gui_gen_qr_btn = "GENERAR QR",
    .gui_files_ready = "%d archivos listos"
};

static i18n_bundle lang_en = {
    .title = "SERVER READY",
    .exit_btn = "END SESSION",
    .donate_text = "Support this project",
    .dl_prefix = "Download",
    .shutdown_msg = "SESSION TERMINATED",
    .gui_drag_label = "DRAG YOUR FILES HERE",
    .gui_add_btn = "ADD FILES",
    .gui_gen_qr_btn = "GENERATE QR",
    .gui_files_ready = "%d files ready"
};

static inline i18n_bundle* get_bundle(const char *http_buffer) {
    if (http_buffer && (strstr(http_buffer, "Accept-Language: en") || strstr(http_buffer, "lang=en"))) {
        return &lang_en;
    }
    return &lang_es;
}

static inline i18n_bundle* get_local_bundle() {
    const char *lang = getenv("LANG");
    if (lang && strncmp(lang, "en", 2) == 0) return &lang_en;
    return &lang_es;
}

#endif