#include "render_target.hh"

/* Re-size the Render Target. */
Result<void> RenderTarget::resize(uint32_t new_width, uint32_t new_height) {
    width = new_width;
    height = new_height;
    return rebuild(); /* Trigger a re-build */
}
