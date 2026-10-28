#include "smp_resource.h"
#include "smp_player.h"

#include <SDL.h>

struct StarfishResource {
    char windowId[32];
    int maxRefreshRate;
    // The window was made by another process (SS4S_SMP_WINDOW_ID), which also places it: this one never touches SDL.
    bool externalWindow;
};

StarfishResource *StarfishResourceCreate(const char *appId) {
    (void) appId;
    StarfishResource *res = calloc(1, sizeof(StarfishResource));
    assert(res != NULL);
    const char *external = getenv("SS4S_SMP_WINDOW_ID");
    if (external != NULL && external[0] != '\0') {
        strncpy(res->windowId, external, sizeof(res->windowId) - 1);
        res->externalWindow = true;
        res->maxRefreshRate = 60;
        return res;
    }
    if (!SDL_webOSGetRefreshRate(&res->maxRefreshRate)) {
        res->maxRefreshRate = 60;
    }
    return res;
}

void StarfishResourceDestroy(StarfishResource *res) {
    if (res->windowId[0] != '\0' && !res->externalWindow) {
        SDL_webOSDestroyExportedWindow(res->windowId);
    }
    free(res);
}

bool StarfishResourcePopulateLoadPayload(StarfishResource *resource, jvalue_ref arg,
                                         const SS4S_AudioInfo *audioInfo, const SS4S_VideoInfo *videoInfo) {
    (void) audioInfo;
    if (videoInfo != NULL) {
        if (resource->windowId[0] == '\0') {
            const char *createdWnd = SDL_webOSCreateExportedWindow(0);
            if (createdWnd == NULL) {
                StarfishLibContext->Log(SS4S_LogLevelError, "SMP", "Didn't get a valid windowId: %s",
                                        resource->windowId);
                return false;
            }
            strncpy(resource->windowId, createdWnd, sizeof(resource->windowId) - 1);
        }
        jvalue_ref option = jobject_get(arg, J_CSTR_TO_BUF("option"));
        jobject_set(option, J_CSTR_TO_BUF("windowId"), j_cstr_to_jval(resource->windowId));
        // Which of the TV's video outputs to play on: main_video (the default) or sub_video, the second hardware
        // window a TV like the LG C1 has beside the main one.
        const char *sink = getenv("SS4S_SMP_VIDEO_SINK");
        if (sink != NULL && sink[0] != '\0') {
            jobject_set(option, J_CSTR_TO_BUF("avSink"), jobject_create_var(
                jkeyval(J_CSTR_TO_JVAL("videoSink"), jobject_create_var(
                    jkeyval(J_CSTR_TO_JVAL("type"), j_cstr_to_jval(sink)),
                    J_END_OBJ_DECL
                )),
                J_END_OBJ_DECL
            ));
        }
    }
    return true;
}

bool StarfishResourceSetMediaId(StarfishResource *resource, const char *connId) {
    (void) resource;
    (void) connId;
    return true;
}

bool StarfishResourceSetMediaAudioData(StarfishResource *resource, const char *data) {
    (void) resource;
    (void) data;
    return true;
}

bool StarfishResourceSetMediaVideoData(StarfishResource *resource, const char *data, bool hdr) {
    (void) resource;
    (void) data;
    (void) hdr;
    return true;
}

bool StarfishResourceLoadCompleted(StarfishResource *resource, const char *mediaId) {
    (void) resource;
    (void) mediaId;
    return true;
}

bool StarfishResourcePostLoad(StarfishResource *resource, const SS4S_VideoInfo *info) {
    if (resource->windowId[0] == '\0') {
        return false;
    }
    if (resource->externalWindow) {
        return true;
    }
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    SDL_Rect src = {0, 0, info->width, info->height};
    SDL_Rect dst = {0, 0, dm.w, dm.h};
    // Fit the video to the screen at its own aspect ratio, letterboxed, rather than stretch it.
    if (info->width > 0 && info->height > 0) {
        if ((long long) info->width * dm.h > (long long) info->height * dm.w) {
            dst.h = (int) ((long long) dm.w * info->height / info->width);
            dst.y = (dm.h - dst.h) / 2;
        } else {
            dst.w = (int) ((long long) dm.h * info->width / info->height);
            dst.x = (dm.w - dst.w) / 2;
        }
    }
    SDL_webOSSetExportedWindow(resource->windowId, &src, &dst);
    return true;
}

bool StarfishResourceStartPlaying(StarfishResource *resource) {
    (void) resource;
    return true;
}

bool StarfishResourcePostUnload(StarfishResource *resource) {
    (void) resource;
    return true;
}
