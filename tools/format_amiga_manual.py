#!/usr/bin/env python3
"""
Formatea Amiga_Hardware_Reference_Manual_1985_Commodore_a.md a Markdown
completo con extensiones soportadas por VSCode (GFM, frontmatter, etc.).
Uso: python format_amiga_manual.py
"""
import re
import sys
from pathlib import Path

# Rutas
REPO = Path(__file__).resolve().parent.parent
SRC = REPO / "Amiga_Hardware_Reference_Manual_1985_Commodore_a.md"
DST = REPO / "Amiga_Hardware_Reference_Manual_1985_Commodore.md"

# Patrones
# "- 2 Introduction -" (número primero) o "- Introduction 5 -" (número antes del último -)
PAGE_FOOTER = re.compile(r'^\s*-\s*(\d+\s+.+|\S.+\s+\d+)\s*-\s*$')
CHAPTER_HEAD = re.compile(r'^\s*Chapter\s+(\d+)\s*$', re.I)
FIGURE_CAPTION = re.compile(r'^(Figure\s+\d+-\d+)\s*:?\s*(.*)$', re.I)
TABLE_CAPTION = re.compile(r'^(Table\s+\d+-\d+)\s*:?\s*(.*)$', re.I)
# TOC: línea con puntos suspensivos y número de página al final
TOC_LINE = re.compile(r'\.{2,}\s*\d+\s*$')

# Líneas que parecen inicio de código asm (68000) — mnemonics reales
ASM_START = re.compile(
    r'^\s*(INCLUDE|MOVE\.[BWL]|LEA\s|DC\.|BRA|BSR|BEQ|BNE|CMP|ADD|SUB|AND|OR|EOR|'
    r'RTS|RTE|NOP|JSR|JMP|MOVEM|DBRA|BCC|BCS|BGE|BGT|BLE|BLT|ASL|ASR|LSL|LSR)\b',
    re.I
)
# Comentario asm
ASM_COMMENT = re.compile(r'^\s*;\s*')
# Solo inicio explícito de bloque C (no texto que contenga "custom." o "external")
C_START = re.compile(r'^\s*#\s*include\s|^\s*main\s*\(|^\s*extern\s+struct|^\s*/\*|^\s*\*\s*/|^\s*\}\s*$')
C_CODE = re.compile(r'^\s*[{}]\s*$|^\s*(if|else|while|for|return)\s*\(|^\s*custom\.\w+\s*=')


def is_asm_line(line: str) -> bool:
    s = line.rstrip()
    if not s:
        return False
    # No considerar líneas de TOC (puntos suspensivos + número)
    if TOC_LINE.search(s) or ('.' * 4 in s and re.search(r'\d{2,3}\s*$', s)):
        return False
    # Línea que empieza con espacios + mnemónico o etiqueta
    if ASM_START.match(s) or ASM_COMMENT.match(s):
        return True
    # DC.W $xxxx,$yyyy o similar
    if re.match(r'^\s+DC\.\w+\s+', s):
        return True
    # Etiqueta al inicio (palabra seguida de :)
    if re.match(r'^\s*[A-Za-z_][A-Za-z0-9_]*\s*:', s):
        return True
    # Línea con 4+ espacios y mnemónico 68000 real (no "Moving" ni "EXAMPLE:")
    if len(line) - len(line.lstrip()) >= 4:
        if re.search(r'\b(MOVE\.|LEA\s|DC\.\w+|BRA\.|BSR\.|RTS|ADD\.|SUBQ)', s, re.I):
            return True
    return False


def is_c_line(line: str) -> bool:
    s = line.strip()
    # Solo líneas que claramente inician código C
    if C_START.match(s) or C_CODE.match(s):
        return True
    if s.startswith('#include') or s.startswith('extern struct'):
        return True
    return False


def is_code_block_start(line: str, next_lines: list) -> bool:
    """Detecta inicio de bloque de código (asm o C)."""
    if is_c_line(line):
        return True
    # No abrir asm para líneas de TOC o títulos de ejemplo
    s = line.strip()
    if TOC_LINE.search(s) or re.match(r'^\s*(EXAMPLE|Moving\s*\(|SUMMARY)\s*:', s, re.I):
        return False
    if is_asm_line(line):
        count = 0
        for L in next_lines[:15]:
            if is_asm_line(L) or (L.strip().startswith(';')):
                count += 1
            elif L.strip() == '' and count > 0:
                break
            elif count >= 2:
                return True
            else:
                break
        return count >= 2
    return False


def detect_code_lang(line: str) -> str:
    if line.strip().startswith('#include') or 'main()' in line or 'extern' in line or 'custom.' in line:
        return 'c'
    return 'asm'


def main():
    src_text = SRC.read_text(encoding='utf-8', errors='replace')
    lines = src_text.replace('\r\n', '\n').replace('\r', '\n').split('\n')

    out = []
    i = 0
    in_code_block = False
    code_lang = 'asm'
    in_list = False

    # Frontmatter YAML (soportado por VSCode y muchas herramientas RAG)
    out.append('---')
    out.append('title: Amiga Hardware Reference Manual')
    out.append('year: 1985')
    out.append('publisher: Commodore')
    out.append('description: Official hardware reference for the Amiga computer (68000, custom chips, graphics, audio, blitter).')
    out.append('---')
    out.append('')

    while i < len(lines):
        line = lines[i]
        rest = lines[i + 1:] if i + 1 < len(lines) else []
        stripped = line.strip()

        # Título principal (solo al inicio)
        if i == 0 and stripped:
            out.append(f'# {stripped}')
            out.append('')
            i += 1
            continue

        # Saltar página footer
        if PAGE_FOOTER.match(stripped):
            i += 1
            continue

        # Form feed -> separador
        if '\x0c' in line:
            line = line.replace('\x0c', '')
            rest_stripped = line.strip()
            if rest_stripped:
                # Si el resto es un encabezado en MAYÚSCAS, formatear como ###
                if 2 <= len(rest_stripped) <= 80 and rest_stripped.isupper() and '|' not in rest_stripped:
                    out.append('')
                    out.append(f'### {rest_stripped}')
                else:
                    out.append(line)
            if not in_code_block:
                out.append('')
                out.append('---')
                out.append('')
            i += 1
            continue

        stripped = line.strip()

        # Cierre de bloque de código
        if in_code_block:
            # Salir si línea en blanco y la siguiente no es código, o si es un párrafo claro
            end_code = False
            if not (is_asm_line(line) or is_c_line(line) or (stripped and stripped.startswith(';'))):
                if stripped == '':
                    # Una línea en blanco: mirar siguiente
                    if rest and (is_asm_line(rest[0]) or is_c_line(rest[0]) or rest[0].strip().startswith(';')):
                        out.append(line)
                    else:
                        end_code = True
                else:
                    end_code = True
            if end_code:
                out.append('```')
                out.append('')
                in_code_block = False
            else:
                out.append(line)
            i += 1
            continue

        # Título "Chapter N" en línea propia, siguiente línea = nombre capítulo
        if CHAPTER_HEAD.match(stripped) and rest:
            num = CHAPTER_HEAD.match(stripped).group(1)
            next_stripped = rest[0].strip() if rest else ''
            if next_stripped and not next_stripped.startswith('.'):
                out.append(f'## Chapter {num}: {next_stripped}')
                out.append('')
                i += 2
                continue

        # Encabezados de sección (línea entera en MAYÚSCAS, no vacía, longitud razonable)
        if stripped and 2 <= len(stripped) <= 80 and stripped.isupper():
            # No convertir si es parte de una tabla o código
            if not is_asm_line(line) and not line.startswith('    ') and '|' not in stripped:
                out.append('')
                out.append(f'### {stripped}')
                out.append('')
                i += 1
                in_list = False
                continue

        # NOTE como blockquote
        if stripped == 'NOTE' and rest:
            out.append('')
            out.append('> **NOTE**')
            i += 1
            # Siguientes líneas hasta línea vacía o otro NOTE/header
            while i < len(lines):
                nl = lines[i]
                if PAGE_FOOTER.match(nl.strip()):
                    i += 1
                    continue
                if nl.strip() == '' or nl.strip() == 'NOTE':
                    break
                out.append('> ' + nl)
                i += 1
            out.append('')
            continue

        # Lista con "o "
        if re.match(r'^(\s*)o\s+', line):
            if not in_list:
                out.append('')
            out.append(re.sub(r'^(\s*)o\s+', r'\1- ', line))
            in_list = True
            i += 1
            continue

        # Figure X-Y: caption
        m = FIGURE_CAPTION.match(stripped)
        if m and not in_code_block:
            out.append('')
            out.append(f'**{m.group(1)}:** {m.group(2).strip()}')
            out.append('')
            i += 1
            continue

        # Table X-Y: caption
        m = TABLE_CAPTION.match(stripped)
        if m and not in_code_block:
            out.append('')
            out.append(f'*{m.group(1)}:* {m.group(2).strip()}')
            out.append('')
            i += 1
            continue

        # Inicio de bloque de código
        if not in_code_block and is_code_block_start(line, rest):
            code_lang = detect_code_lang(line)
            out.append('')
            out.append(f'```{code_lang}')
            out.append(line)
            in_code_block = True
            in_list = False
            i += 1
            continue
        in_list = False

        # Línea normal
        out.append(line)
        i += 1

    if in_code_block:
        out.append('```')
        out.append('')

    result = '\n'.join(out)

    # Limpiezas globales
    # Encabezados "Table OF CONTENTS" -> con formato
    result = re.sub(
        r'^Table OF CONTENTS\s*$',
        '## Table of contents',
        result,
        flags=re.MULTILINE
    )
    # LIST OF FIGURES
    result = re.sub(
        r'^\s*LIST OF FIGURES\s*$',
        '## List of figures',
        result,
        flags=re.MULTILINE
    )
    # Índice / Appendix como sección
    result = re.sub(
        r'^Index\s+\.+[\d\s]*$',
        '## Index',
        result,
        flags=re.MULTILINE
    )
    result = re.sub(
        r'^Glossary\s+\.+[\d\s]*$',
        '## Glossary',
        result,
        flags=re.MULTILINE
    )
    # Appendix X
    result = re.sub(
        r'^Appendix\s+([A-Z])\s+([^\s.]+)\s*\.+[\d\s]*$',
        r'## Appendix \1: \2',
        result,
        flags=re.MULTILINE
    )
    # Eliminar líneas de puntos de TOC (.... 2) dejando solo el título; opcional: convertir a links
    # Dejamos los puntos por ahora para no romper estructura.

    # Reducir líneas en blanco consecutivas a máximo 2
    result = re.sub(r'\n{4,}', '\n\n\n', result)

    DST.write_text(result, encoding='utf-8')
    print(f"Written: {DST}", file=sys.stderr)
    return 0


if __name__ == '__main__':
    sys.exit(main())
