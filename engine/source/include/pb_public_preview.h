#ifndef PB_PUBLIC_PREVIEW_H
#define PB_PUBLIC_PREVIEW_H
#include <stdint.h>
#ifdef __SDCC
#include <gb/gb.h>
#define PBPP_API BANKED
#else
#define PBPP_API
#endif
typedef struct {uint8_t rank,suit,enhancement;} PBPPCard;
enum {PBPP_KNOWN=0,PBPP_UNKNOWN=1,PBPP_INVALID=255};
/* Pinned recognition roles: Four Fingers1, Shortcut2, Smeared4. Invoke only
 * for a sanitized visible, active Joker. Hidden identity/effects are absent. */
uint8_t pbpp_visible_role(uint8_t visible_id) PBPP_API;
/* Selected public face cards only, max5. Stone canonical rank/suit are0.
 * known_profile0..7, hidden_count0..5. Each hidden slot can contribute at most
 * one role in the pinned core; copies do not copy these intrinsic modifiers.
 * At most8 feasible profiles. KNOWN writes a unanimous type0..11; UNKNOWN or
 * INVALID preserves output. Null/alias/noncanonical input rejects. No hidden
 * identity, actual secret-dependent preview, levels, UID or GameState input. */
uint8_t pbpp_consensus(const PBPPCard *,uint8_t selected_count,
                       uint8_t known_profile,uint8_t hidden_count,uint8_t *type) PBPP_API;
#endif
