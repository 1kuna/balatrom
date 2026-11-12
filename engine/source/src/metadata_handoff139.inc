/* Physical bank handoff; caller runs the existing ferry-window admission
 * first. No call/stack access occurs while bank3/2 are selected. Main has
 * ended staging, no game writer can run, and IRQs continue reading bank3.
 * Copy only journaled metadata blocks with short existing16-byte ferries,
 * with coop_active=0 to bypass the prospective bank2 alias. Then use this
 * atomic final synchronization for the sole IRQ-written byte D3B0.
 */
void pub139_metadata_handoff(void) __naked BANKED {
 __asm
 push af
 push bc
 di
 ld a,#3
 ldh (0x70),a
 ld a,(#0xd3b0)
 ld b,a
 ld a,#2
 ldh (0x70),a
 ld a,b
 ld (#0xd3b0),a
 ld a,#2
 ldh (0xec),a
 ld a,#1
 ldh (0x70),a
 ei
 pop bc
 pop af
 ret
 __endasm;
}

/* No invocation from an IRQ: this helper deliberately resumes interrupts
 * only after both selected banks and bank1 stack access are coherent. The
 * enabled state must match the existing main-only CRITICAL ferry contract.
 * OAM shadow is a separate lease: restore FF92=C0 only after all160 new
 * bytes are copied to C000; until then ordinary CRT DMA stays suppressed.
 */
