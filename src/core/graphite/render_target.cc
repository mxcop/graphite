#include "render_target.hh"

/* Re-size the Render Target. */
Result<void> RenderTarget::resize(u32 new_width, u32 new_height) {
    width = new_width;
    height = new_height;
    return rebuild(); /* Trigger a re-build */
}
