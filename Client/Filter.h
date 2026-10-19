#pragma once

// Replaces profanity in a nickname or chat message with '*', in place and
// without changing its length. Letters swapped for look-alike digits or
// symbols (sh1t, a$$) are caught, a word that merely contains another (class,
// Scunthorpe) is left alone.
void rr_filter_censor(char *text);
