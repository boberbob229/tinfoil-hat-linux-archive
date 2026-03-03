/*
 * text2morse.c - ASCII text to morse code token stream
 * Part of morse2led-1.0
 * Compiled with GCC 2.96 (Red Hat Linux 7.1), statically linked against uClibc
 *
 * Converts text to a configurable token stream suitable for piping to blinker.
 * By default outputs tokens 'n' (dot/dit) and 'nd' (dash/dah) compatible
 * with blinker's LED blinking protocol.
 *
 * Reconstructed from binary (version 01.01, stripped ELF32/i386).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* International Morse code table (A-Z, 0-9) */
/* '.' = dit, '-' = dah */
static const char *morse_table[] = {
    /* A */ ".-",
    /* B */ "-...",
    /* C */ "-.-.",
    /* D */ "-..",
    /* E */ ".",
    /* F */ "..-.",
    /* G */ "--.",
    /* H */ "....",
    /* I */ "..",
    /* J */ ".---",
    /* K */ "-.-",
    /* L */ ".-..",
    /* M */ "--",
    /* N */ "-.",
    /* O */ "---",
    /* P */ ".--.",
    /* Q */ "--.-",
    /* R */ ".-.",
    /* S */ "...",
    /* T */ "-",
    /* U */ "..-",
    /* V */ "...-",
    /* W */ ".--",
    /* X */ "-..-",
    /* Y */ "-.--",
    /* Z */ "--..",
    /* 0 */ "-----",
    /* 1 */ ".----",
    /* 2 */ "..---",
    /* 3 */ "...--",
    /* 4 */ "....-",
    /* 5 */ ".....",
    /* 6 */ "-....",
    /* 7 */ "--...",
    /* 8 */ "---..",
    /* 9 */ "----.",
};

static void usage(const char *prog)
{
    fprintf(stderr,
        "USAGE: text2morse { \"text\" | - } ( \"-\" indicates input from stdin )\n"
        "  [-dot \"string\"]\n"
        "  [-dash \"string\"]\n"
        "  [-letter \"between letters\"]\n"
        "  [-space \"between words\"]\n"
        "  [-help] ( more help )\n"
        "HELP:\n"
        "  Examples:\n"
        "\n"
        "  text2morse will convert regular ASCII text input into\n"
        "  a stream of morse code output. Each morse code letter\n"
        "  is built from tokens (DOT, DASH, etc.) you can define:\n"
        "      -dot \"string\" is a symbolic dot, eg. -dot \".\"\n"
        "      -dash \"string\" is a symbolic dash, eg. -dash \"-\"\n"
        "      -letter \"string\" terminates each morse letter\n"
        "      -word \"string\" marks where a space occurs in input\n"
        "  Token defaults are set to work with the blinker program.\n"
        "  Piping default output to blinker will blink DOT on caps\n"
        "  lock and DASH on number locks. Provide a pacing delay to\n"
        "  blinker with the -cd \"msecs\" flag.\n"
        "\n"
        "  cat MY_TEXT | text2morse - | blinker - -cd 80\n"
        "\n"
        "  text2morse \"SOS SOS\" | blinker - -cd 100\n"
        "\n"
        "  text2morse -dot \".\" -dash \"-\" -letter \" \" -word \"  \" SOS\n"
        "\n"
    );
}

static void emit(const char *token)
{
    if (token && *token)
        fputs(token, stdout);
}

static void encode_char(int ch,
                         const char *dot_tok,
                         const char *dash_tok,
                         const char *letter_tok)
{
    const char *code = NULL;

    ch = toupper(ch);
    if (ch >= 'A' && ch <= 'Z')
        code = morse_table[ch - 'A'];
    else if (ch >= '0' && ch <= '9')
        code = morse_table[26 + (ch - '0')];
    else
        return; /* unknown character: skip */

    while (*code) {
        if (*code == '.')
            emit(dot_tok);
        else
            emit(dash_tok);
        code++;
    }
    emit(letter_tok);
}

int main(int argc, char *argv[])
{
    /* Defaults match blinker token format */
    const char *dot_tok    = "n";      /* numlock on/off = dot  */
    const char *dash_tok   = "nd";     /* numlock+delay  = dash */
    const char *letter_tok = "d";      /* delay between letters */
    const char *word_tok   = "dd";     /* double delay for spaces */
    const char *input_str  = NULL;
    int use_stdin = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-dot") == 0 && i + 1 < argc) {
            dot_tok = argv[++i];
        } else if (strcmp(argv[i], "-dash") == 0 && i + 1 < argc) {
            dash_tok = argv[++i];
        } else if (strcmp(argv[i], "-letter") == 0 && i + 1 < argc) {
            letter_tok = argv[++i];
        } else if (strcmp(argv[i], "-word") == 0 && i + 1 < argc) {
            word_tok = argv[++i];
        } else if (strcmp(argv[i], "-") == 0) {
            use_stdin = 1;
        } else if (argv[i][0] != '-') {
            input_str = argv[i];
        } else {
            fprintf(stderr, "text2morse: unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    if (!use_stdin && !input_str) {
        usage(argv[0]);
        return 1;
    }

    if (use_stdin) {
        int ch;
        while ((ch = fgetc(stdin)) != EOF) {
            if (ch == ' ' || ch == '\n' || ch == '\t') {
                emit(word_tok);
            } else {
                encode_char(ch, dot_tok, dash_tok, letter_tok);
            }
        }
    } else {
        const char *p = input_str;
        while (*p) {
            if (*p == ' ' || *p == '\n' || *p == '\t') {
                emit(word_tok);
            } else {
                encode_char(*p, dot_tok, dash_tok, letter_tok);
            }
            p++;
        }
    }

    fflush(stdout);
    return 0;
}
