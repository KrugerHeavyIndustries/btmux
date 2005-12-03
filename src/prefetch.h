#ifndef __PREFETCH_H__
#define __PREFETCH_H__

#define alternative_input(oldinstr, newinstr, feature, input...)        \
    asm volatile ("661:\n\t" oldinstr "\n662:\n"                \
            ".section .altinstructions,\"a\"\n"           \
            "  .align 4\n"                        \
            "  .long 661b\n"            /* label */           \
            "  .long 663f\n"        /* new instruction */     \
            "  .byte %c0\n"             /* feature bit */     \
            "  .byte 662b-661b\n"       /* sourcelen */       \
            "  .byte 664f-663f\n"       /* replacementlen */      \
            ".previous\n"                     \
            ".section .altinstr_replacement,\"ax\"\n"         \
            "663:\n\t" newinstr "\n664:\n"   /* replacement */    \
            ".previous" :: "i" (feature), ##input)


static inline void prefetch(const void *x)
{
    alternative_input(ASM_NOP4,
            "prefetchnta (%1)",
            X86_FEATURE_XMM,
            "r" (x));
}


#endif
