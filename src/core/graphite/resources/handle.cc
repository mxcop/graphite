#include "handle.hh"

#include "graphite/vram_bank.hh"

AgnVRAMBank* move(AgnVRAMBank*& a) {
    AgnVRAMBank* c = a;
    a = nullptr;
    return c;
}

OpaqueHandle::OpaqueHandle(const OpaqueHandle& o) noexcept : index(o.index), type(o.type), bank(o.bank) {
    if (bank != nullptr) bank->inc_handle_refs(*this); /* <- noexcept check */
}

OpaqueHandle& OpaqueHandle::operator=(const OpaqueHandle& o) noexcept {
    if (bank != nullptr) bank->dec_handle_refs(*this);
    index = o.index;
    type = o.type;
    bank = o.bank;
    if (bank != nullptr) bank->inc_handle_refs(*this); /* <- noexcept check */
    return *this;
}

OpaqueHandle::OpaqueHandle(OpaqueHandle&& o) noexcept : index(o.index), type(o.type), bank(move(o.bank)) {}

OpaqueHandle& OpaqueHandle::operator=(OpaqueHandle&& o) noexcept {
    if (bank != nullptr) bank->dec_handle_refs(*this);
    index = o.index;
    type = o.type;
    bank = move(o.bank);
    return *this;
}

OpaqueHandle::~OpaqueHandle() {
    if (bank != nullptr) bank->dec_handle_refs(*this);
}
