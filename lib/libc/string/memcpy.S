/*
 * Minimal libc/runtime piece for bare-metal (string/memcpy.S).
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
# This code is covered by BSD license. It was taken from NetBSD
# common/lib/libc/arch/m68k/string/bcopy.S

#include <asm.h>

# [a0] src address
# [a1] dest address
# [d1] count
ENTRY(memcpy)
#if defined(__ELF__)
        move.l 4(%sp),%a1
        move.l 8(%sp),%a0
        move.l 12(%sp),%d1
#endif
        move.l  a1,-(sp)                /* save dest address */

        cmp.l   a1,a0                   /* src after dest? */
        jlt     .Lbcback                /* yes, must copy backwards */

        /* It isn't worth the overhead of aligning to {long}word boundries */
        /* if the string is too short. */
        cmp.l   #8,d1
        jlt     .Lbcfbyte

        /*
         * The 68010 cannot access a word or long on an odd boundary,
         * period.  If the source and the destination addresses aren't
         * of the same evenness, we're forced to do a bytewise copy.
         */
        move.l  a0,d0
        add.l   a1,d0
        btst    #0,d0
        jne     .Lbcfbyte
        
        /* word align */
        move.l  a1,d0
        btst    #0,d0           /* if (dst & 1) */
        jeq     .Lbcfalgndw
        move.b  (a0)+,(a1)+     /*      *(char *)dst++ = *(char *) src++ */
        subq.l  #1,d1           /*      len-- */
.Lbcfalgndw:
        /* long word align */
        btst    #1,d0           /* if (dst & 2) */
        jeq     .Lbcfalgndl
        move.w  (a0)+,(a1)+     /*      *(short *)dst++ = *(short *) dst++ */
        subq.l  #2,d1           /*      len -= 2 */
.Lbcfalgndl:
        /* copy by 8 longwords */
        move.l  d1,d0
        lsr.l   #5,d0           /* cnt = len / 32 */
        jeq     .Lbcflong       /* if (cnt) */
        and.l   #31,d1          /*      len %= 32 */
        subq.l  #1,d0           /*      set up for dbf */
.Lbcf32loop:
        /*
         * Conceptual C:
         *   for (int i=0; i<8; ++i) *(uint32_t*)dst++ = *(uint32_t*)src++;
         *
         * Unrolled by 8 to reduce branch overhead on 68000/010 where branch
         * penalties are significant versus straight move.l streams.
         */
        move.l  (a0)+,(a1)+     /*      copy 8 long words */
        move.l  (a0)+,(a1)+
        move.l  (a0)+,(a1)+
        move.l  (a0)+,(a1)+
        move.l  (a0)+,(a1)+
        move.l  (a0)+,(a1)+
        move.l  (a0)+,(a1)+
        move.l  (a0)+,(a1)+
        dbf     d0,.Lbcf32loop  /*      till done */
        clr.w   d0
        subq.l  #1,d0
        jcc     .Lbcf32loop

.Lbcflong:
        /* copy by longwords */
        move.l  d1,d0
        lsr.l   #2,d0           /* cnt = len / 4 */
        jeq     .Lbcfbyte       /* if (cnt) */
        subq.l  #1,d0           /*      set up for dbf */
.Lbcflloop:
        move.l  (a0)+,(a1)+     /*      copy longwords */
        dbf     d0,.Lbcflloop   /*      till done */
        and.l   #3,d1           /*      len %= 4 */
        jeq     .Lbcdone

        subq.l  #1,d1           /* set up for dbf */
.Lbcfbloop:
        move.b  (a0)+,(a1)+     /* copy bytes */
.Lbcfbyte:
        dbf     d1,.Lbcfbloop   /* till done */
.Lbcdone:
        move.l  (sp)+,d0        /* dest address */
        rts

.Lbcback:
        /*
         * Backward copy path handles overlap when dst starts inside src range:
         * bytes are copied from end to beginning to avoid clobbering unread src.
         * This is memmove semantics; memcpy alias keeps compatibility in this libc.
         */
        add.l   d1,a0           /* src pointer to end */
        add.l   d1,a1           /* dest pointer to end */

        /* It isn't worth the overhead of aligning to {long}word boundries */
        /* if the string is too short. */
        cmp.l   #8,d1
        jlt     .Lbcbbyte

        /* The 68010 cannot access a word or long on an odd boundary, */
        /* period.  If the source and the destination addresses aren't */
        /* of the same evenness, we're forced to do a bytewise copy. */
        move.l  a0,d0
        add.l   a1,d0
        btst    #0,d0
        jne     .Lbcbbyte
        
        /* word align */
        move.l  a1,d0
        btst    #0,d0           /* if (dst & 1) */
        jeq     .Lbcbalgndw
        move.b  -(a0),-(a1)     /*      *(char *)dst-- = *(char *) src-- */
        subq.l  #1,d1           /*      len-- */
.Lbcbalgndw:
        /* long word align */
        btst    #1,d0           /* if (dst & 2) */
        jeq     .Lbcbalgndl
        move.w  -(a0),-(a1)     /*      *(short *)dst-- = *(short *) dst-- */
        subq.l  #2,d1           /*      len -= 2 */
.Lbcbalgndl:
        /* copy by 8 longwords */
        move.l  d1,d0
        lsr.l   #5,d0           /* cnt = len / 32 */
        jeq     .Lbcblong       /* if (cnt) */
        and.l   #31,d1          /*      len %= 32 */
        subq.l  #1,d0           /*      set up for dbf */
.Lbcb32loop:
        /* Same 8x unroll as forward path, but with pre-decrement addressing. */
        move.l  -(a0),-(a1)     /*      copy 8 long words */
        move.l  -(a0),-(a1)
        move.l  -(a0),-(a1)
        move.l  -(a0),-(a1)
        move.l  -(a0),-(a1)
        move.l  -(a0),-(a1)
        move.l  -(a0),-(a1)
        move.l  -(a0),-(a1)
        dbf     d0,.Lbcb32loop  /*      till done */
        clr.w   d0
        subq.l  #1,d0
        jcc     .Lbcb32loop
        
.Lbcblong:
        /* copy by longwords */
        move.l  d1,d0
        lsr.l   #2,d0           /* cnt = len / 4 */
        jeq     .Lbcbbyte       /* if (cnt) */
        subq.l  #1,d0           /*      set up for dbf */
.Lbcblloop:
        move.l  -(a0),-(a1)     /*      copy longwords */
        dbf     d0,.Lbcblloop   /*      till done */
        and.l   #3,d1           /*      len %= 4 */
        jeq     .Lbcdone

        subq.l  #1,d1           /* set up for dbf */
.Lbcbbloop:
        move.b  -(a0),-(a1)     /* copy bytes */
.Lbcbbyte:
        dbf     d1,.Lbcbbloop   /* till done */

        move.l  (sp)+,d0        /* dest address */
        rts
END(memcpy)

STRONG_ALIAS(memmove,memcpy)
STRONG_ALIAS(bcopy,memcpy)

# vim: ft=gas:ts=8:sw=8:noet:
