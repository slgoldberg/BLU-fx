#!/usr/bin/env python3
# ==============================================================================
# ImGee User Manual & UX Manifesto PDF Generator (tools/generate_manual.py)
# ------------------------------------------------------------------------------
# Author: Steven L. Goldberg (@slgoldberg)
# Publication-grade ReportLab Markdown-to-PDF compiler.
#
# Ingests ImGee Markdown documentation and compiles a clean, publication-grade
# PDF user manual with running headers, "Page X of Y" dynamic two-pass footers,
# callout alert boxes (> [!NOTE]), styled code blocks, and zebra-striped tables.
# ==============================================================================

import os
import sys
import re
import argparse
from typing import List, Tuple

try:
    from reportlab.lib.pagesizes import letter
    from reportlab.lib import colors
    from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
    from reportlab.platypus import (
        SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether, HRFlowable, Preformatted, XPreformatted, Image as RLImage
    )
    from reportlab.pdfgen import canvas
    from reportlab.pdfbase import pdfmetrics
    from reportlab.pdfbase.ttfonts import TTFont
    from PIL import Image as PILImage
except ImportError:
    print("Error: ReportLab is required. Install via: pip install reportlab", file=sys.stderr)
    sys.exit(1)


# ------------------------------------------------------------------------------
# 0. Global TrueType Font Registry
# ------------------------------------------------------------------------------
MONO_FONT = "Courier"
MONO_BOLD_FONT = "Courier-Bold"
BOX_MONO_FONT = None
HAS_BOX_DRAWING = False
HAS_APPLE_SYMBOLS = False
HAS_ARIAL_UNICODE = False

def _load_ttfont(font_name: str, path: str, subfont_index=None) -> TTFont:
    if subfont_index is not None:
        return TTFont(font_name, path, subfontIndex=subfont_index)
    return TTFont(font_name, path)


def init_pdf_fonts():
    global MONO_FONT, MONO_BOLD_FONT, BOX_MONO_FONT, HAS_BOX_DRAWING
    global HAS_APPLE_SYMBOLS, HAS_ARIAL_UNICODE

    script_dir = os.path.dirname(os.path.abspath(__file__))
    tools_fonts_dir = os.path.join(script_dir, "fonts")
    repo_root = os.path.dirname(script_dir)
    sibling_roboto_mono = os.path.normpath(
        os.path.join(repo_root, "..", "imgui4xp-pg-test", "imgui4xp", "fonts", "RobotoMono-Regular.ttf")
    )
    local_rm_medium = os.path.join(tools_fonts_dir, "RobotoMono-Medium.ttf")
    local_rm_bold   = os.path.join(tools_fonts_dir, "RobotoMono-Bold.ttf")

    # 1. Primary modern monospace discovery:
    #    RobotoMono-Medium (500) + RobotoMono-Bold (700) -> Menlo.ttc (0/1) -> RobotoMono-Regular -> DejaVu -> Courier New
    mono_candidates = [
        (local_rm_medium, None, local_rm_bold, None),
        ('/System/Library/Fonts/Menlo.ttc', 0, '/System/Library/Fonts/Menlo.ttc', 1),
        ('/Users/brat/Documents/imgui4xp-pg-test/imgui4xp/fonts/RobotoMono-Regular.ttf', None, local_rm_bold, None),
        (sibling_roboto_mono, None, local_rm_bold, None),
        ('/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf', None, '/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf', None),
        ('/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf', None, '/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf', None),
        ('/System/Library/Fonts/Supplemental/Courier New.ttf', None, '/System/Library/Fonts/Supplemental/Courier New Bold.ttf', None),
        ('/Library/Fonts/Courier New.ttf', None, '/Library/Fonts/Courier New Bold.ttf', None),
        ('C:\\Windows\\Fonts\\cour.ttf', None, 'C:\\Windows\\Fonts\\courbd.ttf', None),
    ]
    for reg_path, reg_sub, bold_path, bold_sub in mono_candidates:
        if reg_path and os.path.exists(reg_path):
            try:
                tt_reg = _load_ttfont('MonoTTF', reg_path, reg_sub)
                pdfmetrics.registerFont(tt_reg)
                MONO_FONT = 'MonoTTF'
                HAS_BOX_DRAWING = (0x251C in tt_reg.face.charWidths and 0x2500 in tt_reg.face.charWidths)
                if bold_path and os.path.exists(bold_path):
                    try:
                        pdfmetrics.registerFont(_load_ttfont('MonoTTFBold', bold_path, bold_sub))
                        MONO_BOLD_FONT = 'MonoTTFBold'
                    except Exception:
                        MONO_BOLD_FONT = 'MonoTTF'
                else:
                    MONO_BOLD_FONT = 'MonoTTF'
                pdfmetrics.registerFontFamily(
                    'MonoTTF',
                    normal='MonoTTF',
                    bold=MONO_BOLD_FONT,
                    italic='MonoTTF',
                    boldItalic=MONO_BOLD_FONT,
                )
                break
            except Exception:
                pass

    # 1b. Secondary sturdy fixed-width box-drawing font (Menlo.ttc Regular/Bold -> DejaVuSansMono -> Courier New)
    if HAS_BOX_DRAWING:
        BOX_MONO_FONT = MONO_FONT
    else:
        box_candidates = [
            ('/System/Library/Fonts/Menlo.ttc', 0, '/System/Library/Fonts/Menlo.ttc', 1),
            ('/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf', None, '/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf', None),
            ('/System/Library/Fonts/Supplemental/Courier New.ttf', None, '/System/Library/Fonts/Supplemental/Courier New Bold.ttf', None),
            ('/Library/Fonts/Courier New.ttf', None, '/Library/Fonts/Courier New Bold.ttf', None),
            ('C:\\Windows\\Fonts\\cour.ttf', None, 'C:\\Windows\\Fonts\\courbd.ttf', None),
        ]
        for bp, b_sub, bp_bold, b_bold_sub in box_candidates:
            if os.path.exists(bp):
                try:
                    tt_box = _load_ttfont('BoxMonoTTF', bp, b_sub)
                    if 0x251C in tt_box.face.charWidths:
                        pdfmetrics.registerFont(tt_box)
                        box_bold_name = 'BoxMonoTTF'
                        if bp_bold and os.path.exists(bp_bold):
                            try:
                                pdfmetrics.registerFont(_load_ttfont('BoxMonoTTFBold', bp_bold, b_bold_sub))
                                box_bold_name = 'BoxMonoTTFBold'
                            except Exception:
                                pass
                        pdfmetrics.registerFontFamily(
                            'BoxMonoTTF',
                            normal='BoxMonoTTF',
                            bold=box_bold_name,
                            italic='BoxMonoTTF',
                            boldItalic=box_bold_name,
                        )
                        BOX_MONO_FONT = 'BoxMonoTTF'
                        break
                except Exception:
                    pass

    # 2. Apple Symbols (gear, info, etc.)
    apple_candidates = [
        '/System/Library/Fonts/Apple Symbols.ttf',
        '/Library/Fonts/Apple Symbols.ttf',
    ]
    for ap in apple_candidates:
        if os.path.exists(ap):
            try:
                pdfmetrics.registerFont(TTFont('AppleSymbols', ap))
                pdfmetrics.registerFontFamily(
                    'AppleSymbols',
                    normal='AppleSymbols',
                    bold='AppleSymbols',
                    italic='AppleSymbols',
                    boldItalic='AppleSymbols',
                )
                HAS_APPLE_SYMBOLS = True
                break
            except Exception:
                pass

    # 3. Arial Unicode / DejaVu Sans (plane, check, arrows, etc.)
    unicode_candidates = [
        '/System/Library/Fonts/Supplemental/Arial Unicode.ttf',
        '/Library/Fonts/Arial Unicode.ttf',
        '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',
        '/usr/share/fonts/truetype/freefont/FreeSans.ttf',
        'C:\\Windows\\Fonts\\ARIALUNI.TTF',
    ]
    for up in unicode_candidates:
        if os.path.exists(up):
            try:
                pdfmetrics.registerFont(TTFont('ArialUnicode', up))
                pdfmetrics.registerFontFamily(
                    'ArialUnicode',
                    normal='ArialUnicode',
                    bold='ArialUnicode',
                    italic='ArialUnicode',
                    boldItalic='ArialUnicode',
                )
                HAS_ARIAL_UNICODE = True
                break
            except Exception:
                pass

init_pdf_fonts()


def format_unicode_symbols(text: str) -> str:
    def wrap_sym(char, font_name, fallback):
        if font_name == 'AppleSymbols' and HAS_APPLE_SYMBOLS:
            return f'<font name="AppleSymbols">{char}</font>'
        elif font_name == 'ArialUnicode' and HAS_ARIAL_UNICODE:
            return f'<font name="ArialUnicode">{char}</font>'
        return fallback

    text = text.replace("⚙", wrap_sym("⚙", "AppleSymbols", "[gear]"))
    text = text.replace("ℹ", wrap_sym("ℹ", "AppleSymbols", "[i]"))
    text = text.replace("↺", wrap_sym("↺", "ArialUnicode", "[undo]"))
    text = text.replace("↻", wrap_sym("↻", "ArialUnicode", "[redo]"))
    text = text.replace("⇄", wrap_sym("⇄", "ArialUnicode", "<->"))
    text = text.replace("✓", wrap_sym("✓", "ArialUnicode", "[check]")).replace("✔", wrap_sym("✔", "ArialUnicode", "[check]"))
    text = text.replace("✗", wrap_sym("✗", "ArialUnicode", "[x]")).replace("✘", wrap_sym("✘", "ArialUnicode", "[x]"))
    text = text.replace("✈", wrap_sym("✈", "ArialUnicode", "[plane]"))
    text = text.replace("★", wrap_sym("★", "ArialUnicode", "*"))
    text = text.replace("▲", wrap_sym("▲", "ArialUnicode", "^"))
    text = text.replace("▼", wrap_sym("▼", "ArialUnicode", "v"))
    text = text.replace("◀", wrap_sym("◀", "ArialUnicode", "<"))
    text = text.replace("▶", wrap_sym("▶", "ArialUnicode", ">"))
    text = text.replace("→", wrap_sym("→", "ArialUnicode", "->"))
    text = text.replace("←", wrap_sym("←", "ArialUnicode", "<-"))
    text = text.replace("Δ", wrap_sym("Δ", "ArialUnicode", "Delta"))
    text = text.replace("•", wrap_sym("•", "ArialUnicode", "*"))
    return text


# ------------------------------------------------------------------------------
# 1. Two-Pass Numbered Canvas (Running Header & "Page X of Y" Footer)
# ------------------------------------------------------------------------------
def make_numbered_canvas(header_text: str = "ImGee: Modern Ergonomic UI Toolkit for Dear ImGui -- User Guide",
                         footer_left: str = "ImGee for Dear ImGui • Author: Steven L. Goldberg (@slgoldberg)"):
    """
    Factory creating a two-pass canvas that accumulates total page count before drawing
    running headers, rules, and dynamic 'Page X of Y' footers.
    """
    class DynamicNumberedCanvas(canvas.Canvas):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self._saved_page_states = []

        def showPage(self):
            self._saved_page_states.append(dict(self.__dict__))
            self._startPage()

        def save(self):
            num_pages = len(self._saved_page_states)
            for state in self._saved_page_states:
                self.__dict__.update(state)
                self.draw_page_decorations(num_pages)
                canvas.Canvas.showPage(self)
            canvas.Canvas.save(self)

        def draw_page_decorations(self, page_count):
            if self._pageNumber == 1:
                # Suppress header and footer on cover / title page
                return

            self.saveState()
            self.setFont("Helvetica", 8)
            self.setFillColor(colors.HexColor("#64748B"))

            # Running Header
            self.drawString(54, 750, header_text)
            self.setStrokeColor(colors.HexColor("#CBD5E1"))
            self.setLineWidth(0.5)
            self.line(54, 744, 558, 744)

            # Running Footer
            self.line(54, 48, 558, 48)
            footer_right = f"Page {self._pageNumber} of {page_count}"
            self.drawString(54, 36, footer_left)
            self.drawRightString(558, 36, footer_right)

            self.restoreState()

    return DynamicNumberedCanvas


NumberedCanvas = make_numbered_canvas()


# ------------------------------------------------------------------------------
# 2. Markdown Inline Parser & HTML Sanitizer
# ------------------------------------------------------------------------------
def sanitize_inline_markdown(text: str) -> str:
    """
    Converts Markdown inline syntax (**bold**, *italic*, `code`, ~~strike~~, ++ins++)
    to ReportLab-compatible HTML markup safely.
    """
    # 1. Extract and protect backtick inline code spans with unique placeholders
    code_spans = []
    def save_code(m):
        code_content = m.group(1).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        code_content = format_unicode_symbols(code_content)
        idx = len(code_spans)
        code_spans.append(f'<font name="{MONO_FONT}" size="-0.5" color="#1E40AF">{code_content}</font>')
        return f"___CODE_SPAN_{idx}___"

    text = re.sub(r'`([^`]+)`', save_code, text)

    # 2. Escape raw XML ampersands & format symbols
    text = text.replace("&", "&amp;")
    text = format_unicode_symbols(text)

    # 2b. Bold-Italic (***text*** or ___text___) before standard bold / italic:
    text = re.sub(r'\*\*\*([^\*\s][^\*]*?[^\*\s]|\S)\*\*\*', r'<b><i>\1</i></b>', text)
    text = re.sub(r'(?<!\w)___(?!CODE_SPAN_)([^_\s].*?[^_\\s]|\S)___(?!\w)', r'<b><i>\1</i></b>', text)

    # 3. Bold: **bold**
    text = re.sub(r'\*\*(.+?)\*\*', r'<b>\1</b>', text)

    # 4. Italic: *italic* (avoid matching solitary asterisks or bullet starts)
    text = re.sub(r'(?<!\*)\*([^\*\s][^\*]*?[^\*\s]|\S)\*(?!\*)', r'<i>\1</i>', text)

    # 5. Italic via underscores: only when bounded by whitespace/punctuation (never intra-word like FOO_BAR_BAZ)
    text = re.sub(r'(?<=\s)_([^_]+?)_(?=\s|[.,;:!?\)]|$)', r'<i>\1</i>', text)

    # 6. Underline: ++text++ (strict word boundary to prevent C++ / C++11 / C++17 false positive)
    text = re.sub(r'(?<![a-zA-Z0-9_])\+\+([^\+\s].*?[^\+\s]|\S)\+\+(?![a-zA-Z0-9_])', r'<u>\1</u>', text)

    # 7. Strikethrough: ~~text~~ or $$strike$$
    text = re.sub(r'~~([^~]+?)~~', r'<font color="#94A3B8">\1</font>', text)
    text = re.sub(r'\$\$([^\$]+?)\$\$', r'<font color="#94A3B8">\1</font>', text)

    # 8. Links: [label](url)
    def link_repl(m):
        label, url = m.group(1), m.group(2)
        if url.startswith("http://") or url.startswith("https://") or url.startswith("mailto:"):
            return f'<link href="{url}" color="#2563EB"><u>{label}</u></link>'
        else:
            # Internal anchor or relative file reference (safe fallback without broken PDF destination)
            return f'<font color="#2563EB"><u>{label}</u></font>'
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', link_repl, text)

    # 9. Transform or sanitize custom tags (<badge>, <pill>, <kbd>, <key>, <kdb>, <br>)
    text = re.sub(r'<clear(?:\s+[^>]*)?/?>', '', text, flags=re.IGNORECASE)
    text = re.sub(r'<br\s+clear\s*=\s*["\']?[a-zA-Z]+["\']?\s*/?>', '<br/>', text, flags=re.IGNORECASE)
    text = re.sub(r'<br\s*/?>', '<br/>', text, flags=re.IGNORECASE)
    text = re.sub(
        r'<(?:key|kbd|kdb)(?:\s+[^>]*)?>(.+?)</(?:key|kbd|kdb)>',
        r'<font name="Helvetica-Bold" color="#0F172A"><b>[&nbsp;\1&nbsp;]</b></font>',
        text,
        flags=re.IGNORECASE,
    )
    valid_tags = {'b', '/b', 'i', '/i', 'u', '/u', 'font', '/font', 'link', '/link', 'a', '/a', 'br', 'br/'}
    def tag_filter(m):
        raw = m.group(0)
        tag_name = m.group(1).split()[0].lower()
        if tag_name in valid_tags or tag_name.startswith('font ') or tag_name.startswith('link '):
            return raw
        if any(tag_name.startswith(k) for k in ('badge', 'pill', 'kbd', 'tag', 'btn', 'key', 'code', 'mono')):
            return f'<font name="Helvetica-Bold" color="#1E293B">[{raw[1:-1]}]</font>'
        return raw.replace("<", "&lt;").replace(">", "&gt;")

    text = re.sub(r'</?([a-zA-Z0-9_\-=:#,/ ]+)>', tag_filter, text)

    # 10. Escape any remaining stray angle brackets
    text = re.sub(r'<(?!(?:b|/b|i|/i|u|/u|font|/font|link|/link|a|/a|br/)(?:\s|>))', '&lt;', text)

    # 11. Restore code spans
    for idx, span in enumerate(code_spans):
        text = text.replace(f"___CODE_SPAN_{idx}___", span)

    return text


def safe_paragraph(raw_text: str, style) -> Paragraph:
    """
    Safely creates a Paragraph flowable with graceful plain-text fallback
    if malformed markup is encountered.
    """
    formatted = sanitize_inline_markdown(raw_text)
    if getattr(style, 'name', '') == 'TableHeader':
        formatted = formatted.replace('color="#1E40AF"', 'color="#DBEAFE"')
    try:
        return Paragraph(formatted, style)
    except Exception:
        plain = re.sub(r'<[^>]+>', '', raw_text)
        plain = plain.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        return Paragraph(plain, style)


# ------------------------------------------------------------------------------
# 2b. Single-Pass GitHub-Light Syntax Highlighter for XPreformatted Code Cards
# ------------------------------------------------------------------------------
LANG_HEADER_LABELS = {
    'cpp': 'C++',
    'c++': 'C++',
    'cxx': 'C++',
    'cc': 'C++',
    'h': 'C++',
    'hpp': 'C++',
    'hxx': 'C++',
    'c': 'C',
    'lua': 'Lua',
    'cmake': 'CMake',
    'bash': 'Shell',
    'sh': 'Shell',
    'zsh': 'Shell',
    'shell': 'Shell',
    'python': 'Python',
    'py': 'Python',
    'markdown': 'Markdown',
    'md': 'Markdown',
    'html': 'HTML',
    'xml': 'XML',
    'json': 'JSON',
    'ini': 'Config',
}

GH_COLOR_DEFAULT  = "#1F2328"  # Deep charcoal
GH_COLOR_COMMENT  = "#6E7781"  # Muted slate
GH_COLOR_PREPROC  = "#CF222E"  # Crimson
GH_COLOR_STRING   = "#0A3069"  # Deep navy
GH_COLOR_KEYWORD  = "#8250DF"  # Rich violet
GH_COLOR_API_TYPE = "#0550AE"  # Crisp royal blue
GH_COLOR_CONSTANT = "#953800"  # Warm amber-brown
GH_COLOR_TREE     = "#334155"  # Clean un-highlighted charcoal for ASCII trees

CODE_KEYWORDS = {
    # C / C++
    'auto', 'bool', 'break', 'case', 'catch', 'char', 'class', 'const',
    'const_cast', 'constexpr', 'continue', 'default', 'delete', 'do',
    'double', 'dynamic_cast', 'else', 'enum', 'explicit', 'extern',
    'false', 'final', 'float', 'for', 'friend', 'goto', 'if', 'inline',
    'int', 'long', 'mutable', 'namespace', 'new', 'noexcept', 'nullptr',
    'operator', 'override', 'private', 'protected', 'public', 'register',
    'reinterpret_cast', 'return', 'short', 'signed', 'sizeof', 'static',
    'static_assert', 'static_cast', 'struct', 'switch', 'template',
    'this', 'throw', 'true', 'try', 'typedef', 'typename', 'union',
    'unsigned', 'using', 'virtual', 'void', 'volatile', 'wchar_t', 'while',
    'size_t', 'ptrdiff_t', 'int32_t', 'uint32_t', 'int64_t', 'uint64_t',
    # Lua
    'local', 'function', 'end', 'then', 'elseif', 'repeat', 'until',
    'in', 'and', 'or', 'not', 'nil',
    # Python / Shell
    'def', 'import', 'from', 'as', 'with', 'except', 'finally', 'raise',
    'pass', 'lambda', 'yield', 'None', 'True', 'False', 'fi', 'done',
    'esac', 'export', 'elif',
}

CODE_KNOWN_TYPES = {
    'ImGui', 'ImGee', 'ImgWindow', 'TimedTooltip', 'Markdown', 'MD',
    'Typography', 'MarkdownImageService', 'Detail', 'std',
    'ImVec2', 'ImVec4', 'ImU32', 'ImGuiID', 'ImFont', 'ImFontAtlas',
    'ImFontRole', 'ImDrawList', 'ImGuiIO', 'ImGuiStyle', 'ImGuiStorage',
    'ImGuiContext', 'ImGuiWindow', 'ImTextureID',
    'ImGuiWindowFlags', 'ImGuiChildFlags', 'ImGuiKnobFlags',
    'ImGuiSelectableFlags', 'ImGuiHoveredFlags', 'ImGuiMouseCursor',
    'ImGuiPopupAnchor', 'ImGuiPopupAlign', 'ImGuiBadgeVariant',
    'LinkClickResult', 'ScopedFontRole', 'TooltipConfig', 'TooltipState',
    'MarkdownConfig', 'MarkdownTableState', 'MarkdownFormatInfo',
    'MarkdownLinkCallbackData', 'ZoomableViewportState',
    'DocumentViewerState', 'DocumentViewerConfig', 'DocumentTocEntry',
    'HostBackend', 'Config', 'State', 'HeadingFormat', 'TagCallback',
    'CustomTagCallback', 'LinkCallback', 'TooltipCallback',
    'ImageCallback', 'FormatCallback', 'string', 'vector', 'unordered_map',
    'pair',
}

CONSTANT_PREFIXES = (
    'IMGEE_', 'CONTRIB_', 'IM_', 'IMGUI_', 'ICON_', 'GL_',
    'ImGuiCol_', 'ImGuiStyleVar_', 'ImGuiCond_', 'ImGuiMouseCursor_',
    'ImGuiBadgeVariant_', 'ImGuiPopupAnchor_', 'ImGuiPopupAlign_',
    'DocumentViewerTheme_',
)

_CPP_TOKEN_RE = re.compile(
    r'(?P<block_comment>/\*[\s\S]*?\*/)'
    r'|(?P<line_comment>//[^\n]*)'
    r'|(?P<raw_string>R"([A-Za-z0-9_]{0,16})\([\s\S]*?\)\2")'
    r'|(?P<inc_line>#[ \t]*include[ \t]*(?:<[^>\n]+>|"[^"\n]+"))'
    r'|(?P<preproc>#[ \t]*(?:include|define|ifdef|ifndef|if|elif|else|endif|pragma|undef|error)\b)'
    r'|(?P<string>"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\')'
    r'|(?P<number>(?<![A-Za-z0-9_])(?:0[xX][0-9a-fA-F\']+[uUlL]*|(?:[0-9]+\.[0-9]*|\.[0-9]+)(?:[eE][+-]?[0-9]+)?[fFlL]?|[0-9][0-9\']*(?:[eE][+-]?[0-9]+)?[uUlLfF]*))'
    r'|(?P<ident>[A-Za-z_][A-Za-z0-9_]*)'
    r'|(?P<ws>[ \t]+)'
    r'|(?P<nl>\n)'
    r'|(?P<other>.)'
)

_SCRIPT_TOKEN_RE = re.compile(
    r'(?P<lua_block_comment>--\[\[[\s\S]*?\]\])'
    r'|(?P<lua_comment>--[^\n]*)'
    r'|(?P<hash_comment>#[^\n]*)'
    r'|(?P<triple_string>"""[\s\S]*?"""|\'\'\'[\s\S]*?\'\'\'|\[\[[\s\S]*?\]\])'
    r'|(?P<string>"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\')'
    r'|(?P<number>(?<![A-Za-z0-9_])(?:0[xX][0-9a-fA-F]+|(?:[0-9]+\.[0-9]*|\.[0-9]+)(?:[eE][+-]?[0-9]+)?|[0-9]+))'
    r'|(?P<ident>[A-Za-z_][A-Za-z0-9_]*)'
    r'|(?P<ws>[ \t]+)'
    r'|(?P<nl>\n)'
    r'|(?P<other>.)'
)

_MARKUP_TOKEN_RE = re.compile(
    r'(?P<html_comment><!--[\s\S]*?-->)'
    r'|(?P<callout>\[!(?:NOTE|INFO|TIP|IMPORTANT|WARNING|CAUTION)\])'
    r'|(?P<heading>(?:^|(?<=\n))[ \t]*#{1,6}[ \t]+[^\n]*)'
    r'|(?P<inline_code>`[^`\n]+`)'
    r'|(?P<string>"[^"\n]*"|\'[^\'\n]*\')'
    r'|(?P<tag_open></?[A-Za-z][A-Za-z0-9_\-:]*)'
    r'|(?P<tag_close>/?\s*>)'
    r'|(?P<attr>\b[A-Za-z_][A-Za-z0-9_\-]*(?=\s*=))'
    r'|(?P<ws>[ \t]+)'
    r'|(?P<nl>\n)'
    r'|(?P<other>[^<>"\'`\s#\[\n]+|.)'
)


def _escape_code_xml(text: str, use_box_font: bool = False) -> str:
    esc = text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    if not use_box_font:
        esc = format_unicode_symbols(esc)
    return esc


def _is_constant_ident(ident: str) -> bool:
    if ident in ('NULL', 'FLT_MAX', 'INT_MAX', 'UINT_MAX'):
        return True
    if ident.startswith(CONSTANT_PREFIXES):
        return True
    if 'Flags_' in ident:
        return True
    if len(ident) >= 3 and ident.isupper() and any(c.isalpha() for c in ident):
        return True
    return False


def tokenize_code_block(code: str, lang_raw: str) -> Tuple[List[Tuple[str, str, bool]], bool]:
    """
    Tokenizes a code block into (raw_token_text, hex_color, is_bold) spans.
    Returns (tokens, is_box_tree).
    """
    lang = (lang_raw or "").strip().lower().split()[0] if (lang_raw or "").strip() else ""
    has_box_chars = any(ch in code for ch in ("├", "└", "│", "─", "┌", "┐", "┘"))
    if has_box_chars or lang in ("", "text", "txt", "plaintext", "ascii"):
        return [(code, GH_COLOR_TREE, False)], has_box_chars

    tokens: List[Tuple[str, str, bool]] = []

    if lang in ("markdown", "md", "html", "xml"):
        for m in _MARKUP_TOKEN_RE.finditer(code):
            kind = m.lastgroup
            val = m.group(0)
            if kind == "html_comment":
                tokens.append((val, GH_COLOR_COMMENT, False))
            elif kind in ("callout", "heading"):
                tokens.append((val, GH_COLOR_API_TYPE, True))
            elif kind in ("tag_open", "tag_close"):
                tokens.append((val, GH_COLOR_API_TYPE, False))
            elif kind in ("inline_code", "string"):
                tokens.append((val, GH_COLOR_STRING, False))
            elif kind == "attr":
                tokens.append((val, GH_COLOR_KEYWORD, False))
            else:
                tokens.append((val, GH_COLOR_DEFAULT, False))
        return tokens, False

    is_cpp_like = lang in ("cpp", "c++", "cxx", "cc", "c", "h", "hpp", "hxx")
    is_lua = (lang == "lua")
    regex = _CPP_TOKEN_RE if is_cpp_like else _SCRIPT_TOKEN_RE

    for m in regex.finditer(code):
        kind = m.lastgroup
        val = m.group(0)
        end_pos = m.end()

        if kind in ("block_comment", "line_comment", "lua_block_comment", "lua_comment"):
            tokens.append((val, GH_COLOR_COMMENT, False))
        elif kind == "hash_comment":
            if is_lua:
                tokens.append((val, GH_COLOR_DEFAULT, False))
            else:
                tokens.append((val, GH_COLOR_COMMENT, False))
        elif kind == "inc_line":
            inc_m = re.match(r'^(#[ \t]*include)([ \t]*)(<[^>\n]+>|"[^"\n]+")$', val)
            if inc_m:
                tokens.append((inc_m.group(1), GH_COLOR_PREPROC, True))
                if inc_m.group(2):
                    tokens.append((inc_m.group(2), GH_COLOR_DEFAULT, False))
                tokens.append((inc_m.group(3), GH_COLOR_STRING, False))
            else:
                tokens.append((val, GH_COLOR_PREPROC, True))
        elif kind == "preproc":
            tokens.append((val, GH_COLOR_PREPROC, True))
        elif kind in ("raw_string", "triple_string", "string"):
            tokens.append((val, GH_COLOR_STRING, False))
        elif kind == "number":
            tokens.append((val, GH_COLOR_CONSTANT, False))
        elif kind == "ident":
            if val in CODE_KEYWORDS:
                tokens.append((val, GH_COLOR_KEYWORD, True))
            elif _is_constant_ident(val):
                tokens.append((val, GH_COLOR_CONSTANT, False))
            elif val in CODE_KNOWN_TYPES:
                tokens.append((val, GH_COLOR_API_TYPE, False))
            else:
                # Look ahead past horizontal whitespace for '(' or '::', or look behind for '::'
                rest = code[end_pos:]
                la_m = re.match(r'^[ \t]*(\(|::)', rest)
                prev_two = code[max(0, m.start() - 2):m.start()]
                if la_m or prev_two == "::":
                    tokens.append((val, GH_COLOR_API_TYPE, False))
                else:
                    tokens.append((val, GH_COLOR_DEFAULT, False))
        else:
            tokens.append((val, GH_COLOR_DEFAULT, False))

    return tokens, False


def build_highlighted_code_lines(code_lines: List[str], code_lang: str) -> Tuple[List[str], bool, bool]:
    """
    Converts raw code lines into XML-formatted XPreformatted lines with syntax highlighting
    and clean soft-wrapping for ultra-wide (> 100-char) lines so text never overflows card borders.
    Returns (xml_lines, use_compact_font, use_box_font).
    """
    expanded_lines = [ln.replace("\t", "    ") for ln in code_lines]
    raw_joined = "\n".join(expanded_lines)

    has_box_chars = any(ch in raw_joined for ch in ("├", "└", "│", "─", "┌", "┐", "┘"))
    use_box_font = False
    if has_box_chars:
        if BOX_MONO_FONT is not None:
            use_box_font = True
        else:
            raw_joined = (raw_joined
                .replace("├──", "|--")
                .replace("└──", "\\--")
                .replace("│", "|")
                .replace("─", "-")
            )

    max_line_len = max((len(ln) for ln in raw_joined.splitlines()), default=0)
    use_compact_font = (max_line_len > 88)
    max_cols = 100 if use_compact_font else 90

    raw_tokens, _ = tokenize_code_block(raw_joined, code_lang)

    # Split multi-line tokens (such as /* ... */ or R"(...)") at '\n' into logical lines of (text, color, is_bold)
    logical_lines: List[List[Tuple[str, str, bool]]] = [[]]
    for tok_txt, tok_col, tok_bold in raw_tokens:
        if "\n" not in tok_txt:
            if tok_txt:
                logical_lines[-1].append((tok_txt, tok_col, tok_bold))
        else:
            parts = tok_txt.split("\n")
            for p_idx, part in enumerate(parts):
                if p_idx > 0:
                    logical_lines.append([])
                if part:
                    logical_lines[-1].append((part, tok_col, tok_bold))

    # Soft-wrap only lines exceeding max_cols (preserving token colors, bold weight & indentation)
    wrapped_lines: List[List[Tuple[str, str, bool]]] = []
    for line_spans in logical_lines:
        total_len = sum(len(s) for s, _, _ in line_spans)
        if total_len <= max_cols or has_box_chars:
            wrapped_lines.append(line_spans)
            continue

        full_plain = "".join(s for s, _, _ in line_spans)
        lead_spaces = len(full_plain) - len(full_plain.lstrip(" "))
        cont_indent = " " * (min(lead_spaces, 20) + 4)

        # Sub-split long spans on spaces/commas so wrap points exist
        fine_spans: List[Tuple[str, str, bool]] = []
        for s_txt, s_col, s_bold in line_spans:
            for piece in re.findall(r'[^ ,]+[ ,]*|[ ,]+', s_txt):
                if piece:
                    fine_spans.append((piece, s_col, s_bold))

        cur_row: List[Tuple[str, str, bool]] = []
        cur_len = 0
        for piece, p_col, p_bold in fine_spans:
            trimmed = piece.rstrip(" ")
            stripped_piece = piece.lstrip(" ")
            is_tiny_punct = bool(stripped_piece) and len(stripped_piece) <= 3 and all(c in ");,]}>" for c in stripped_piece)
            limit_cols = (max_cols + 2) if is_tiny_punct else max_cols
            if cur_row and cur_len + len(trimmed) > limit_cols and cur_len > len(cont_indent) + 8:
                wrapped_lines.append(cur_row)
                cur_row = [(cont_indent, GH_COLOR_DEFAULT, False)]
                cur_len = len(cont_indent)
                if stripped_piece:
                    cur_row.append((stripped_piece, p_col, p_bold))
                    cur_len += len(stripped_piece)
            else:
                cur_row.append((piece, p_col, p_bold))
                cur_len += len(piece)
        if cur_row:
            wrapped_lines.append(cur_row)

    # Coalesce adjacent same-style spans per line & emit clean XPreformatted XML
    xml_lines: List[str] = []
    for row in wrapped_lines:
        if not row:
            xml_lines.append("")
            continue
        merged: List[Tuple[str, str, bool]] = []
        for s_txt, s_col, s_bold in row:
            if merged and merged[-1][1] == s_col and merged[-1][2] == s_bold:
                merged[-1] = (merged[-1][0] + s_txt, s_col, s_bold)
            else:
                merged.append((s_txt, s_col, s_bold))

        parts_xml = []
        for s_txt, s_col, s_bold in merged:
            esc = _escape_code_xml(s_txt, use_box_font=use_box_font)
            if s_txt.strip() == "":
                parts_xml.append(esc)
            elif s_bold:
                parts_xml.append(f'<font color="{s_col}"><b>{esc}</b></font>')
            else:
                parts_xml.append(f'<font color="{s_col}">{esc}</font>')
        xml_lines.append("".join(parts_xml))

    return xml_lines, use_compact_font, use_box_font


# ------------------------------------------------------------------------------
# 3. Document Parser: Markdown Blocks to ReportLab Flowables
# ------------------------------------------------------------------------------
def build_styles():
    styles = getSampleStyleSheet()

    navy = colors.HexColor("#1E3A8A")
    slate_body = colors.HexColor("#334155")

    styles.add(ParagraphStyle(
        name='ManualTitle',
        fontName='Helvetica-Bold',
        fontSize=24,
        leading=30,
        textColor=navy,
        spaceAfter=6
    ))

    styles.add(ParagraphStyle(
        name='ManualSubtitle',
        fontName='Helvetica',
        fontSize=11,
        leading=15,
        textColor=colors.HexColor("#475569"),
        spaceAfter=16
    ))

    styles['Heading1'].fontName = 'Helvetica-Bold'
    styles['Heading1'].fontSize = 17
    styles['Heading1'].leading = 21
    styles['Heading1'].textColor = navy
    styles['Heading1'].spaceBefore = 14
    styles['Heading1'].spaceAfter = 8
    styles['Heading1'].keepWithNext = True

    styles['Heading2'].fontName = 'Helvetica-Bold'
    styles['Heading2'].fontSize = 12.5
    styles['Heading2'].leading = 16
    styles['Heading2'].textColor = colors.HexColor("#1E293B")
    styles['Heading2'].spaceBefore = 11
    styles['Heading2'].spaceAfter = 6
    styles['Heading2'].keepWithNext = True

    styles['Heading3'].fontName = 'Helvetica-Bold'
    styles['Heading3'].fontSize = 10.5
    styles['Heading3'].leading = 14
    styles['Heading3'].textColor = colors.HexColor("#334155")
    styles['Heading3'].spaceBefore = 8
    styles['Heading3'].spaceAfter = 4
    styles['Heading3'].keepWithNext = True

    styles['BodyText'].fontName = 'Helvetica'
    styles['BodyText'].fontSize = 9.0
    styles['BodyText'].leading = 13.0
    styles['BodyText'].textColor = slate_body
    styles['BodyText'].spaceAfter = 5

    styles.add(ParagraphStyle(
        name='BulletItem',
        fontName='Helvetica',
        fontSize=9.0,
        leading=13.0,
        textColor=slate_body,
        leftIndent=14,
        firstLineIndent=-10,
        spaceAfter=3
    ))

    styles.add(ParagraphStyle(
        name='NumberedItem',
        fontName='Helvetica',
        fontSize=9.0,
        leading=13.0,
        textColor=slate_body,
        leftIndent=18,
        firstLineIndent=-14,
        spaceAfter=3
    ))

    styles.add(ParagraphStyle(
        name='CodeSnippet',
        fontName=MONO_FONT,
        fontSize=8.6,
        leading=11.8,
        textColor=colors.HexColor("#1F2328")
    ))

    styles.add(ParagraphStyle(
        name='CodeSnippetCompact',
        fontName=MONO_FONT,
        fontSize=7.8,
        leading=10.8,
        textColor=colors.HexColor("#1F2328")
    ))

    styles.add(ParagraphStyle(
        name='CodeTreeSnippet',
        fontName=BOX_MONO_FONT or MONO_FONT,
        fontSize=8.4,
        leading=11.4,
        textColor=colors.HexColor("#334155")
    ))

    styles.add(ParagraphStyle(
        name='CodeTreeSnippetCompact',
        fontName=BOX_MONO_FONT or MONO_FONT,
        fontSize=7.1,
        leading=10.0,
        textColor=colors.HexColor("#334155")
    ))

    styles.add(ParagraphStyle(
        name='CodeLangHeader',
        fontName='Helvetica-Bold',
        fontSize=7.2,
        leading=9.0,
        textColor=colors.HexColor("#475569")
    ))

    styles.add(ParagraphStyle(
        name='AlertTitle',
        fontName='Helvetica-Bold',
        fontSize=9.0,
        leading=11.5,
        spaceAfter=3
    ))

    styles.add(ParagraphStyle(
        name='AlertBody',
        fontName='Helvetica',
        fontSize=8.5,
        leading=11.5,
        textColor=slate_body
    ))

    styles.add(ParagraphStyle(
        name='TableHeader',
        fontName='Helvetica-Bold',
        fontSize=8.0,
        leading=10.5,
        textColor=colors.white
    ))

    styles.add(ParagraphStyle(
        name='TableCell',
        fontName='Helvetica',
        fontSize=7.5,
        leading=10.0,
        textColor=slate_body
    ))

    styles.add(ParagraphStyle(
        name='BlockquoteText',
        fontName='Helvetica-Oblique',
        fontSize=8.5,
        leading=12.0,
        textColor=colors.HexColor("#1E293B"),
        spaceAfter=4
    ))

    styles.add(ParagraphStyle(
        name='FigCaption',
        fontName='Helvetica-Oblique',
        fontSize=7.5,
        leading=9.5,
        textColor=colors.HexColor("#64748B"),
        alignment=1, # Centered
        spaceBefore=3
    ))

    return styles


def parse_markdown_to_flowables(md_content: str, styles, is_first_doc: bool = False, has_title_banner: bool = False, base_dir: str = "") -> List:
    root_flowables = []
    flowables = root_flowables
    in_keep_together = False
    lines = md_content.splitlines()
    i = 0
    total_lines = len(lines)

    # If first document and a title banner was emitted, skip redundant leading title/subtitle/hr
    if is_first_doc and has_title_banner:
        while i < total_lines:
            line_s = lines[i].strip()
            if not line_s:
                i += 1
            elif line_s.startswith("# ") or line_s.startswith("## ") or re.match(r'^\s*[-*_]{3,}\s*$', line_s):
                i += 1
            else:
                break

    in_code_block = False
    code_lines = []
    code_lang = ""
    current_fence_char = ""
    current_fence_len = 0

    in_alert = False
    alert_type = ""
    alert_lines = []

    in_table = False
    table_rows = []

    CONTENT_WIDTH = 490.0

    in_blockquote = False
    blockquote_paragraphs = []
    current_quote_lines = []

    def flush_blockquote():
        nonlocal in_blockquote, blockquote_paragraphs, current_quote_lines
        if current_quote_lines:
            blockquote_paragraphs.append(" ".join(current_quote_lines).strip())
            current_quote_lines = []
        if not blockquote_paragraphs:
            in_blockquote = False
            return

        flowable_paragraphs = [safe_paragraph(p, styles['BlockquoteText']) for p in blockquote_paragraphs if p]
        if not flowable_paragraphs:
            in_blockquote = False
            blockquote_paragraphs = []
            return

        accent = colors.HexColor("#0284C7")
        bg = colors.HexColor("#F8FAFC")

        tbl = Table([[flowable_paragraphs]], colWidths=[CONTENT_WIDTH])
        tbl.setStyle(TableStyle([
            ('BACKGROUND', (0, 0), (-1, -1), bg),
            ('LINELEFT', (0, 0), (0, -1), 2.5, accent),
            ('TOPPADDING', (0, 0), (-1, -1), 4),
            ('BOTTOMPADDING', (0, 0), (-1, -1), 4),
            ('LEFTPADDING', (0, 0), (-1, -1), 10),
            ('RIGHTPADDING', (0, 0), (-1, -1), 8),
        ]))
        flowables.append(Spacer(1, 4))
        flowables.append(tbl)
        flowables.append(Spacer(1, 6))

        in_blockquote = False
        blockquote_paragraphs = []
        current_quote_lines = []

    def flush_alert():
        nonlocal in_alert, alert_type, alert_lines
        if not alert_lines:
            in_alert = False
            return

        theme_colors = {
            "NOTE": (colors.HexColor("#2563EB"), colors.HexColor("#EFF6FF")),
            "TIP": (colors.HexColor("#059669"), colors.HexColor("#ECFDF5")),
            "IMPORTANT": (colors.HexColor("#7C3AED"), colors.HexColor("#F5F3FF")),
            "WARNING": (colors.HexColor("#D97706"), colors.HexColor("#FFFBEB")),
            "CAUTION": (colors.HexColor("#DC2626"), colors.HexColor("#FEF2F2")),
        }
        accent, bg = theme_colors.get(alert_type, (colors.HexColor("#475569"), colors.HexColor("#F1F5F9")))

        alert_text = " ".join(alert_lines).strip()
        body_p = safe_paragraph(alert_text, styles['AlertBody'])

        title_style = ParagraphStyle(
            'AlertHeaderSpecific',
            parent=styles['AlertTitle'],
            textColor=accent
        )
        title_p = Paragraph(f"<b>{alert_type}</b>", title_style)

        table_data = [[title_p], [body_p]]
        tbl = Table(table_data, colWidths=[CONTENT_WIDTH])
        tbl.setStyle(TableStyle([
            ('BACKGROUND', (0, 0), (-1, -1), bg),
            ('LINELEFT', (0, 0), (0, -1), 3.0, accent),
            ('TOPPADDING', (0, 0), (-1, -1), 4),
            ('BOTTOMPADDING', (0, 0), (-1, -1), 4),
            ('LEFTPADDING', (0, 0), (-1, -1), 10),
            ('RIGHTPADDING', (0, 0), (-1, -1), 8),
        ]))
        flowables.append(Spacer(1, 4))
        flowables.append(tbl)
        flowables.append(Spacer(1, 6))

        in_alert = False
        alert_type = ""
        alert_lines = []

    def flush_table():
        nonlocal in_table, table_rows
        if not table_rows:
            in_table = False
            return

        filtered_rows = []
        for r in table_rows:
            if all(re.match(r'^\s*[:\-]+\s*$', cell) for cell in r if cell):
                continue
            filtered_rows.append(r)

        if not filtered_rows:
            in_table = False
            table_rows = []
            return

        num_cols = max(len(r) for r in filtered_rows)
        if num_cols > 1:
            raw_weights = []
            for c_idx in range(num_cols):
                max_len = 4
                for r in filtered_rows:
                    cell_txt = re.sub(r'[`\*_]', '', r[c_idx]) if c_idx < len(r) else ""
                    if len(cell_txt) > max_len:
                        max_len = len(cell_txt)
                # Pad short/medium columns slightly so monospace code cells don't wrap on the last token
                raw_weights.append(max(5.0, min(float(max_len) + (4.0 if max_len >= 8 else 0.0), 44.0)))
            total_weight = sum(raw_weights)
            col_widths = [max(26.0, CONTENT_WIDTH * (w / total_weight)) for w in raw_weights]
            scale_w = CONTENT_WIDTH / sum(col_widths)
            col_widths = [cw * scale_w for cw in col_widths]
        else:
            col_widths = [CONTENT_WIDTH]

        formatted_data = []
        for row_idx, row in enumerate(filtered_rows):
            padded_row = row + [""] * (num_cols - len(row))
            row_cells = []
            for cell in padded_row:
                style_to_use = styles['TableHeader'] if row_idx == 0 else styles['TableCell']
                row_cells.append(safe_paragraph(cell, style_to_use))
            formatted_data.append(row_cells)

        tbl = Table(formatted_data, colWidths=col_widths, repeatRows=1)
        t_style = [
            ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor("#1E3A8A")),
            ('ALIGN', (0, 0), (-1, -1), 'LEFT'),
            ('VALIGN', (0, 0), (-1, -1), 'TOP'),
            ('GRID', (0, 0), (-1, -1), 0.5, colors.HexColor("#CBD5E1")),
            ('TOPPADDING', (0, 0), (-1, -1), 3),
            ('BOTTOMPADDING', (0, 0), (-1, -1), 3),
            ('LEFTPADDING', (0, 0), (-1, -1), 4),
            ('RIGHTPADDING', (0, 0), (-1, -1), 4),
        ]
        for r_i in range(1, len(formatted_data)):
            if r_i % 2 == 0:
                t_style.append(('BACKGROUND', (0, r_i), (-1, r_i), colors.HexColor("#F8FAFC")))
        tbl.setStyle(TableStyle(t_style))

        flowables.append(Spacer(1, 4))
        flowables.append(tbl)
        flowables.append(Spacer(1, 6))

        in_table = False
        table_rows = []

    while i < total_lines:
        line = lines[i]

        # Fenced code block (``` or ~~~)
        fence_match = re.match(r'^\s*(`{3,}|~{3,})(.*)$', line)
        if fence_match:
            fence_chars = fence_match.group(1)
            fence_char = fence_chars[0]
            fence_len = len(fence_chars)
            fence_info = fence_match.group(2).strip()

            if in_code_block:
                # Closing fence must match the opening fence character and be at least as long
                if fence_char == current_fence_char and fence_len >= current_fence_len:
                    xml_lines, use_compact, use_box = build_highlighted_code_lines(code_lines, code_lang)
                    if use_box:
                        code_style = styles['CodeTreeSnippetCompact'] if use_compact else styles['CodeTreeSnippet']
                    else:
                        code_style = styles['CodeSnippetCompact'] if use_compact else styles['CodeSnippet']

                    # Guarantee zero horizontal border bleed (e.g. wide un-wrapped ASCII directory trees)
                    inner_card_w = CONTENT_WIDTH - 22.0
                    max_pt_w = 0.0
                    for xln in xml_lines:
                        plain_ln = (re.sub(r'<[^>]+>', '', xln)
                                    .replace('&lt;', '<').replace('&gt;', '>').replace('&amp;', '&'))
                        try:
                            w_pt = pdfmetrics.stringWidth(plain_ln, code_style.fontName, code_style.fontSize)
                        except Exception:
                            w_pt = len(plain_ln) * code_style.fontSize * 0.6
                        if w_pt > max_pt_w:
                            max_pt_w = w_pt
                    if max_pt_w > inner_card_w and max_pt_w > 0:
                        scaled_fs = max(6.2, round(code_style.fontSize * (inner_card_w / max_pt_w), 2))
                        code_style = ParagraphStyle(
                            name=f"{code_style.name}_Fit_{int(scaled_fs * 100)}",
                            parent=code_style,
                            fontSize=scaled_fs,
                            leading=round(scaled_fs * 1.40, 2),
                        )

                    lang_key = (code_lang or "").strip().lower().split()[0] if (code_lang or "").strip() else ""
                    header_label = "" if use_box else LANG_HEADER_LABELS.get(lang_key, "")

                    CHUNK_SIZE = 22
                    table_data = []
                    has_header_row = False
                    if header_label:
                        table_data.append([Paragraph(f"<b>{header_label}</b>", styles['CodeLangHeader'])])
                        has_header_row = True

                    code_row_start = len(table_data)
                    if not xml_lines:
                        table_data.append([XPreformatted("", code_style)])
                    else:
                        for chunk_idx in range(0, len(xml_lines), CHUNK_SIZE):
                            chunk_xml = "\n".join(xml_lines[chunk_idx:chunk_idx + CHUNK_SIZE])
                            try:
                                table_data.append([XPreformatted(chunk_xml, code_style)])
                            except Exception:
                                plain_chunk = re.sub(r'<[^>]+>', '', chunk_xml)
                                table_data.append([Preformatted(plain_chunk, code_style)])

                    code_row_end = len(table_data) - 1
                    card_style = [
                        ('BACKGROUND', (0, 0), (-1, -1), colors.HexColor("#F6F8FA")),
                        ('BOX', (0, 0), (-1, -1), 0.75, colors.HexColor("#D0D7DE")),
                        ('LEFTPADDING', (0, 0), (-1, -1), 10),
                        ('RIGHTPADDING', (0, 0), (-1, -1), 10),
                        ('TOPPADDING', (0, code_row_start), (-1, code_row_end), 0),
                        ('BOTTOMPADDING', (0, code_row_start), (-1, code_row_end), 0),
                        ('TOPPADDING', (0, code_row_start), (-1, code_row_start), 6),
                        ('BOTTOMPADDING', (0, code_row_end), (-1, code_row_end), 6),
                    ]
                    if has_header_row:
                        card_style.extend([
                            ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor("#EAEEF2")),
                            ('LINEBELOW', (0, 0), (-1, 0), 0.5, colors.HexColor("#D0D7DE")),
                            ('TOPPADDING', (0, 0), (-1, 0), 3.5),
                            ('BOTTOMPADDING', (0, 0), (-1, 0), 3.5),
                            ('NOSPLIT', (0, 0), (-1, min(1, code_row_end))),
                        ])

                    tbl = Table(
                        table_data,
                        colWidths=[CONTENT_WIDTH],
                        repeatRows=(1 if has_header_row and len(table_data) > 2 else 0),
                        spaceBefore=4,
                        spaceAfter=6,
                    )
                    tbl.setStyle(TableStyle(card_style))
                    flowables.append(tbl)

                    in_code_block = False
                    code_lines = []
                    code_lang = ""
                    current_fence_char = ""
                    current_fence_len = 0
                    i += 1
                    continue
                else:
                    # Nested code fence inside code block (e.g. ``` inside ````)
                    code_lines.append(line)
                    i += 1
                    continue
            else:
                if in_alert: flush_alert()
                if in_blockquote: flush_blockquote()
                if in_table: flush_table()
                in_code_block = True
                current_fence_char = fence_char
                current_fence_len = fence_len
                code_lang = fence_info
                code_lines = []
                i += 1
                continue

        if in_code_block:
            code_lines.append(line)
            i += 1
            continue

        # Alert Callouts (> [!NOTE])
        alert_match = re.match(r'^\s*>\s*\[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\]\s*(.*)$', line, re.IGNORECASE)
        if alert_match:
            if in_alert: flush_alert()
            if in_blockquote: flush_blockquote()
            if in_table: flush_table()
            in_alert = True
            alert_type = alert_match.group(1).upper()
            initial_text = alert_match.group(2).strip()
            alert_lines = [initial_text] if initial_text else []
            i += 1
            continue

        if in_alert:
            if re.match(r'^\s*>', line):
                alert_lines.append(re.sub(r'^\s*>\s?', '', line))
                i += 1
                continue
            else:
                flush_alert()

        # Standard Blockquotes (> text)
        quote_match = re.match(r'^\s*>\s?(.*)$', line)
        if quote_match:
            if in_alert: flush_alert()
            if in_table: flush_table()
            if not in_blockquote:
                in_blockquote = True
                blockquote_paragraphs = []
                current_quote_lines = []

            quote_text = quote_match.group(1).strip()
            if not quote_text:
                if current_quote_lines:
                    blockquote_paragraphs.append(" ".join(current_quote_lines).strip())
                    current_quote_lines = []
            else:
                current_quote_lines.append(quote_text)
            i += 1
            continue

        if in_blockquote:
            flush_blockquote()

        # Tables (| cell | cell |)
        if line.strip().startswith("|") and line.strip().endswith("|"):
            if not in_table:
                in_table = True
                table_rows = []
            cells = [c.strip() for c in line.strip().split("|")[1:-1]]
            table_rows.append(cells)
            i += 1
            continue
        elif in_table:
            flush_table()

        if not line.strip():
            i += 1
            continue

        # Page Breaks (<!-- pagebreak -->, <!-- newpage -->, \newpage, \pagebreak, <pagebreak />, or --- <newpage ...>)
        if re.match(r'^\s*(?:<!--\s*(?:pagebreak|newpage)\s*-->|\\pagebreak|\\newpage|<pagebreak\s*/?>|[-*_]{3,}\s*<new\s*page.*?>)\s*$', line, re.IGNORECASE):
            flowables.append(PageBreak())
            i += 1
            continue

        # KeepTogether Scopes (<!-- keep-together --> ... <!-- /keep-together -->)
        if re.match(r'^\s*<!--\s*keep[-_]?together\s*-->\s*$', line, re.IGNORECASE):
            if not in_keep_together:
                in_keep_together = True
                flowables = []
            i += 1
            continue
        if re.match(r'^\s*<!--\s*/keep[-_]?together\s*-->\s*$', line, re.IGNORECASE):
            if in_keep_together:
                if in_alert: flush_alert()
                if in_blockquote: flush_blockquote()
                if in_table: flush_table()
                if flowables:
                    root_flowables.append(KeepTogether(flowables))
                flowables = root_flowables
                in_keep_together = False
            i += 1
            continue

        # Ignore other standalone HTML comments (<!-- ... -->)
        if re.match(r'^\s*<!--.*-->\s*$', line):
            i += 1
            continue

        # Standalone float-clear or line-break HTML tags (<br clear="all"/>, <clear/>, etc.)
        if re.match(r'^\s*<(?:br|clear)(?:\s+[^>]*)?/?>\s*(?:</(?:br|clear)>\s*)?$', line, re.IGNORECASE):
            i += 1
            continue

        # Horizontal Rule
        if re.match(r'^\s*[-*_]{3,}\s*$', line):
            flowables.append(Spacer(1, 4))
            flowables.append(HRFlowable(width="100%", thickness=0.5, color=colors.HexColor("#CBD5E1"), spaceAfter=8, spaceBefore=4))
            i += 1
            continue

        # Headings
        if line.startswith("# "):
            flowables.append(safe_paragraph(line[2:].strip(), styles['Heading1']))
            i += 1
            continue
        elif line.startswith("## "):
            flowables.append(safe_paragraph(line[3:].strip(), styles['Heading2']))
            i += 1
            continue
        elif line.startswith("### "):
            flowables.append(safe_paragraph(line[4:].strip(), styles['Heading3']))
            i += 1
            continue
        elif line.startswith("#### "):
            flowables.append(safe_paragraph(f"<b>{line[5:].strip()}</b>", styles['BodyText']))
            i += 1
            continue

        # Bullet List Items
        bullet_match = re.match(r'^\s*[\*\-\+]\s+(.*)$', line)
        if bullet_match:
            flowables.append(safe_paragraph(f"• {bullet_match.group(1).strip()}", styles['BulletItem']))
            i += 1
            continue

        # Numbered List Items
        num_match = re.match(r'^\s*(\d+)[\.\)]\s+(.*)$', line)
        if num_match:
            item_num = num_match.group(1)
            flowables.append(safe_paragraph(f"{item_num}. {num_match.group(2).strip()}", styles['NumberedItem']))
            i += 1
            continue

        # Markdown Images (![Caption](path){attrs}) & Standalone HTML <img .../> tags
        def parse_image_spec(raw_ln: str):
            html_m = re.match(
                r'^\s*(?:<(?:p|div|center)(?:\s+[^>]*)?>\s*)?<img\b([^>]*?)/?>\s*(?:</(?:p|div|center)>\s*)?$',
                raw_ln,
                re.IGNORECASE,
            )
            if html_m:
                attrs_str = html_m.group(1)
                spec = {
                    'src': '',
                    'caption': '',
                    'suppress_caption': False,
                    'pct_mod': None,
                    'px_w_mod': None,
                    'px_h_mod': None,
                    'float_side': None,
                }
                align_val = None
                explicit_float = False
                for am in re.finditer(r'([a-zA-Z0-9_\-]+)(?:\s*=\s*(?:"([^"]*)"|\'([^\']*)\'|([^\s/>]+)))?', attrs_str):
                    k = am.group(1).lower()
                    has_eq = (am.group(2) is not None or am.group(3) is not None or am.group(4) is not None)
                    v = (am.group(2) if am.group(2) is not None else (am.group(3) if am.group(3) is not None else (am.group(4) or ""))).strip()
                    vl = v.lower()
                    if k == 'src':
                        spec['src'] = v
                    elif k in ('alt', 'title') and has_eq and not spec['caption']:
                        spec['caption'] = v
                    elif k in ('caption', 'figcaption'):
                        if has_eq and vl in ('0', 'false', 'none', 'off'):
                            spec['suppress_caption'] = True
                        elif has_eq and v and vl not in ('1', 'true', 'on', 'auto'):
                            spec['caption'] = v
                    elif k in ('nocaption', 'no-caption', 'no_caption'):
                        spec['suppress_caption'] = True
                    elif k in ('width', 'w') and v:
                        if v.endswith('%'):
                            try:
                                spec['pct_mod'] = float(v[:-1]) / 100.0
                            except ValueError:
                                pass
                        else:
                            v_clean = re.sub(r'px$', '', v, flags=re.IGNORECASE)
                            try:
                                spec['px_w_mod'] = float(v_clean)
                            except ValueError:
                                pass
                    elif k in ('height', 'h') and v:
                        v_clean = re.sub(r'px$', '', v, flags=re.IGNORECASE)
                        try:
                            spec['px_h_mod'] = float(v_clean)
                        except ValueError:
                            pass
                    elif k == 'align' and vl in ('left', 'right', 'center', 'centered', 'centre', 'none', 'inline'):
                        align_val = vl
                    elif k in ('float-left', 'float_left', 'wrap-left', 'wrap_left'):
                        spec['float_side'] = 'left'
                        explicit_float = True
                    elif k in ('float-right', 'float_right', 'wrap-right', 'wrap_right'):
                        spec['float_side'] = 'right'
                        explicit_float = True
                    elif k in ('nofloat', 'no-float', 'no_float', 'nowrap', 'no-wrap'):
                        spec['float_side'] = None
                        explicit_float = True
                    elif k in ('float', 'wrap'):
                        if vl in ('left', 'right'):
                            spec['float_side'] = vl
                            explicit_float = True
                        elif vl in ('none', 'off', 'false', '0'):
                            spec['float_side'] = None
                            explicit_float = True
                if not explicit_float and align_val in ('left', 'right'):
                    spec['float_side'] = align_val
                return spec if spec['src'] else None

            md_m = re.match(
                r'^\s*(?:<center>\s*)?!\[([^\]]*)\]\(([^)]+)\)(?:\{([^}]*)\})?\s*(?:</center>\s*)?$',
                raw_ln,
                re.IGNORECASE,
            )
            if not md_m:
                return None

            caption_text = md_m.group(1).strip()
            img_target = md_m.group(2).strip()
            attr_block = (md_m.group(3) or "").strip()

            extra_attr_str = ""
            if img_target.startswith('<') and '>' in img_target:
                gt_idx = img_target.find('>')
                inner_target = img_target[1:gt_idx].strip()
                after_gt = img_target[gt_idx + 1:].strip()
                if after_gt:
                    extra_attr_str += " " + after_gt
                img_target = inner_target

            raw_parts = img_target.split()
            tail_mods = []
            mod_prefixes = (
                '=', 'width=', 'w=', 'height=', 'h=', 'align=', 'float=', 'wrap=',
                'gap=', 'margin=', 'hgap=', 'vgap=', 'border=', 'scale=', 'caption=', 'figcaption='
            )
            mod_flags = {
                'left', 'right', 'center', 'centered', 'centre',
                'float-left', 'float_left', 'wrap-left', 'wrap_left',
                'float-right', 'float_right', 'wrap-right', 'wrap_right',
                'nofloat', 'no-float', 'no_float', 'nowrap', 'no-wrap',
                'caption', 'figcaption', 'nocaption', 'no-caption', 'no_caption',
                'lightbox', 'nolightbox', 'no-lightbox', 'zoom', 'expand', 'nozoom', 'fixed', 'border'
            }
            while len(raw_parts) > 1 and (
                raw_parts[-1].lower().startswith(mod_prefixes)
                or raw_parts[-1].lower() in mod_flags
            ):
                tail_mods.insert(0, raw_parts.pop())
            img_rel_path = " ".join(raw_parts) if raw_parts else ""
            combined_mods = " ".join(tail_mods) + " " + extra_attr_str + " " + attr_block

            spec = {
                'src': img_rel_path,
                'caption': caption_text,
                'suppress_caption': False,
                'pct_mod': None,
                'px_w_mod': None,
                'px_h_mod': None,
                'float_side': None,
            }
            for am in re.finditer(r'(=[^\s}]+|[a-zA-Z0-9_\-]+)(?:\s*=\s*(?:"([^"]*)"|\'([^\']*)\'|([^\s}]+)))?', combined_mods):
                tok_k = am.group(1)
                k = tok_k.lower()
                has_eq = (am.group(2) is not None or am.group(3) is not None or am.group(4) is not None)
                v = (am.group(2) if am.group(2) is not None else (am.group(3) if am.group(3) is not None else (am.group(4) or ""))).strip()
                vl = v.lower()
                if tok_k.startswith('='):
                    dim = tok_k[1:].strip()
                    if dim.endswith('%'):
                        try:
                            spec['pct_mod'] = float(dim[:-1]) / 100.0
                        except ValueError:
                            pass
                    elif 'x' in dim.lower():
                        sw, sh = dim.lower().split('x', 1)
                        try:
                            if sw: spec['px_w_mod'] = float(sw)
                            if sh: spec['px_h_mod'] = float(sh)
                        except ValueError:
                            pass
                    else:
                        try:
                            spec['px_w_mod'] = float(dim)
                        except ValueError:
                            pass
                elif k in ('width', 'w') and v:
                    if v.endswith('%'):
                        try:
                            spec['pct_mod'] = float(v[:-1]) / 100.0
                        except ValueError:
                            pass
                    else:
                        v_clean = re.sub(r'px$', '', v, flags=re.IGNORECASE)
                        try:
                            spec['px_w_mod'] = float(v_clean)
                        except ValueError:
                            pass
                elif k in ('height', 'h') and v:
                    v_clean = re.sub(r'px$', '', v, flags=re.IGNORECASE)
                    try:
                        spec['px_h_mod'] = float(v_clean)
                    except ValueError:
                        pass
                elif k in ('caption', 'figcaption'):
                    if has_eq and vl in ('0', 'false', 'none', 'off'):
                        spec['suppress_caption'] = True
                    elif has_eq and v and vl not in ('1', 'true', 'on', 'auto'):
                        spec['caption'] = v
                elif k in ('nocaption', 'no-caption', 'no_caption'):
                    spec['suppress_caption'] = True
                elif k in ('float-left', 'float_left', 'wrap-left', 'wrap_left'):
                    spec['float_side'] = 'left'
                elif k in ('float-right', 'float_right', 'wrap-right', 'wrap_right'):
                    spec['float_side'] = 'right'
                elif k in ('nofloat', 'no-float', 'no_float', 'nowrap', 'no-wrap'):
                    spec['float_side'] = None
                elif k in ('float', 'wrap'):
                    if vl in ('left', 'right'):
                        spec['float_side'] = vl
                    elif vl in ('none', 'off', 'false', '0'):
                        spec['float_side'] = None
            return spec if spec['src'] else None

        img_spec = parse_image_spec(line)
        if img_spec:
            if in_alert: flush_alert()
            if in_blockquote: flush_blockquote()
            if in_table: flush_table()

            img_rel_path = img_spec['src']
            caption_text = "" if img_spec['suppress_caption'] else img_spec['caption']
            pct_mod = img_spec['pct_mod']
            px_w_mod = img_spec['px_w_mod']
            px_h_mod = img_spec['px_h_mod']
            float_side = img_spec['float_side']

            if base_dir and not os.path.isabs(img_rel_path):
                img_full_path = os.path.normpath(os.path.join(base_dir, img_rel_path))
            else:
                img_full_path = img_rel_path

            if os.path.exists(img_full_path):
                try:
                    with PILImage.open(img_full_path) as pimg:
                        orig_w, orig_h = pimg.size

                    aspect = (float(orig_h) / float(orig_w)) if orig_w > 0 else 1.0

                    if float_side in ('left', 'right'):
                        max_float_w = CONTENT_WIDTH * 0.45
                        css_to_pt = CONTENT_WIDTH / 850.0
                        if pct_mod is not None and pct_mod > 0.0:
                            target_w = min(max_float_w, CONTENT_WIDTH * pct_mod)
                            target_h = target_w * aspect
                        elif px_w_mod is not None or px_h_mod is not None:
                            if px_w_mod and px_h_mod:
                                target_w = px_w_mod * css_to_pt
                                target_h = px_h_mod * css_to_pt
                            elif px_w_mod:
                                target_w = px_w_mod * css_to_pt
                                target_h = target_w * aspect
                            else:
                                target_h = px_h_mod * css_to_pt
                                target_w = (target_h / aspect) if aspect > 0 else target_h
                            if target_w > max_float_w:
                                s = max_float_w / float(target_w)
                                target_w *= s
                                target_h *= s
                        else:
                            target_w = min(max_float_w, orig_w * 0.5)
                            target_h = target_w * aspect

                        max_float_h = 240.0
                        if target_h > max_float_h:
                            sh = max_float_h / float(target_h)
                            target_w *= sh
                            target_h = max_float_h
                    else:
                        max_w = CONTENT_WIDTH - 20
                        if pct_mod is not None and pct_mod > 0.0:
                            target_w = min(max_w, max_w * pct_mod)
                            target_h = target_w * aspect
                        elif px_w_mod is not None or px_h_mod is not None:
                            if px_w_mod and px_h_mod:
                                target_w, target_h = px_w_mod, px_h_mod
                            elif px_w_mod:
                                target_w = px_w_mod
                                target_h = target_w * aspect
                            else:
                                target_h = px_h_mod
                                target_w = (target_h / aspect) if aspect > 0 else target_h
                            if target_w > max_w:
                                s = max_w / float(target_w)
                                target_w *= s
                                target_h *= s
                        else:
                            scale = min(1.0, max_w / float(orig_w))
                            target_w = orig_w * scale
                            target_h = orig_h * scale

                        max_h = 320.0
                        if target_h > max_h:
                            scale_h = max_h / float(target_h)
                            target_w *= scale_h
                            target_h = max_h

                    rl_img = RLImage(img_full_path, width=target_w, height=target_h)
                    cell_elements = [rl_img]
                    if caption_text:
                        cap_fmt = sanitize_inline_markdown(caption_text)
                        cap_fmt = cap_fmt.replace('color="#1E40AF"', 'color="#93C5FD"')
                        try:
                            cell_elements.append(Paragraph(f"<i>{cap_fmt}</i>", styles['FigCaption']))
                        except Exception:
                            cap_plain = caption_text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
                            cell_elements.append(Paragraph(f"<i>{cap_plain}</i>", styles['FigCaption']))

                    fig_box_w = target_w + 4.0
                    fig_tbl = Table([[cell_elements]], colWidths=[fig_box_w], spaceBefore=4, spaceAfter=6)
                    fig_tbl.hAlign = 'CENTER'
                    fig_tbl.setStyle(TableStyle([
                        ('BOX', (0, 0), (-1, -1), 0.75, colors.HexColor("#334155")),
                        ('BACKGROUND', (0, 0), (-1, -1), colors.HexColor("#0F172A")),
                        ('ALIGN', (0, 0), (-1, -1), 'CENTER'),
                        ('TOPPADDING', (0, 0), (-1, -1), 2),
                        ('BOTTOMPADDING', (0, 0), (-1, -1), 3),
                        ('LEFTPADDING', (0, 0), (-1, -1), 2),
                        ('RIGHTPADDING', (0, 0), (-1, -1), 2),
                    ]))

                    if float_side in ('left', 'right'):
                        gutter_w = 12.0
                        prose_col_w = max(180.0, CONTENT_WIDTH - fig_box_w - gutter_w)
                        fig_box_h = target_h + (22.0 if caption_text else 8.0)
                        prose_flowables = []
                        accum_prose_h = 0.0
                        j = i + 1
                        while j < total_lines:
                            nxt = lines[j]
                            nxt_s = nxt.strip()
                            if not nxt_s:
                                j += 1
                                continue
                            # Stop before pagebreak / keep-together / clear comments:
                            if re.match(r'^\s*(?:<!--\s*(?:pagebreak|newpage|/?keep[-_]?together|clear\b).*?-->|\\pagebreak|\\newpage|<pagebreak\s*/?>)\s*$', nxt, re.IGNORECASE):
                                break
                            # Skip benign HTML comments:
                            if re.match(r'^\s*<!--.*-->\s*$', nxt):
                                j += 1
                                continue
                            # Stop before structural blocks, headings, fences, tables, quotes/alerts, clear tags, or images:
                            if (
                                re.match(r'^\s*#{1,6}\s+', nxt)
                                or re.match(r'^\s*[-*_]{3,}\s*$', nxt)
                                or re.match(r'^\s*(`{3,}|~{3,})', nxt)
                                or re.match(r'^\s*>', nxt)
                                or (nxt_s.startswith("|") and nxt_s.endswith("|"))
                                or re.match(r'^\s*<(?:br|clear)(?:\s+[^>]*)?/?>\s*(?:</(?:br|clear)>\s*)?$', nxt, re.IGNORECASE)
                                or parse_image_spec(nxt) is not None
                            ):
                                break

                            bm = re.match(r'^\s*[\*\-\+]\s+(.*)$', nxt)
                            nm = re.match(r'^\s*(\d+)[\.\)]\s+(.*)$', nxt)
                            if bm:
                                p_txt = f"• {bm.group(1).strip()}"
                                p_style = styles['BulletItem']
                            elif nm:
                                p_txt = f"{nm.group(1)}. {nm.group(2).strip()}"
                                p_style = styles['NumberedItem']
                            else:
                                p_txt = nxt_s
                                p_style = styles['BodyText']

                            p_flow = safe_paragraph(p_txt, p_style)
                            prose_flowables.append(p_flow)
                            _, est_h = safe_paragraph(p_txt, p_style).wrap(prose_col_w, 10000.0)
                            accum_prose_h += est_h + getattr(p_style, 'spaceAfter', 4.0)
                            j += 1

                            # Once wrapped prose reaches the floated figure height, let remaining paragraphs
                            # continue below at full 100% page width (natural wrap-around reflow)!
                            if accum_prose_h >= fig_box_h + 4.0:
                                break

                        if prose_flowables:
                            fig_tbl.spaceBefore = 0
                            fig_tbl.spaceAfter = 0
                            if float_side == 'right':
                                side_tbl = Table(
                                    [[prose_flowables, fig_tbl]],
                                    colWidths=[prose_col_w + gutter_w, fig_box_w],
                                    spaceBefore=3,
                                    spaceAfter=5,
                                )
                                side_tbl.setStyle(TableStyle([
                                    ('VALIGN', (0, 0), (-1, -1), 'TOP'),
                                    ('LEFTPADDING', (0, 0), (-1, -1), 0),
                                    ('RIGHTPADDING', (0, 0), (0, 0), gutter_w),
                                    ('RIGHTPADDING', (1, 0), (1, 0), 0),
                                    ('TOPPADDING', (0, 0), (-1, -1), 0),
                                    ('BOTTOMPADDING', (0, 0), (-1, -1), 0),
                                ]))
                            else:
                                side_tbl = Table(
                                    [[fig_tbl, prose_flowables]],
                                    colWidths=[fig_box_w + gutter_w, prose_col_w],
                                    spaceBefore=3,
                                    spaceAfter=5,
                                )
                                side_tbl.setStyle(TableStyle([
                                    ('VALIGN', (0, 0), (-1, -1), 'TOP'),
                                    ('LEFTPADDING', (0, 0), (-1, -1), 0),
                                    ('RIGHTPADDING', (0, 0), (0, 0), gutter_w),
                                    ('RIGHTPADDING', (1, 0), (1, 0), 0),
                                    ('TOPPADDING', (0, 0), (-1, -1), 0),
                                    ('BOTTOMPADDING', (0, 0), (-1, -1), 0),
                                ]))
                            flowables.append(side_tbl)
                            i = j
                            continue

                    flowables.append(KeepTogether(fig_tbl))
                except Exception as e:
                    print(f"[PDF Generator] Warning: Failed to embed image {img_full_path}: {e}")
            else:
                print(f"[PDF Generator] Warning: Image not found: {img_full_path}")
            i += 1
            continue

        # Standard Paragraph
        flowables.append(safe_paragraph(line.strip(), styles['BodyText']))
        i += 1

    if in_alert: flush_alert()
    if in_blockquote: flush_blockquote()
    if in_table: flush_table()
    if in_keep_together and flowables is not root_flowables:
        if flowables:
            root_flowables.append(KeepTogether(flowables))
        flowables = root_flowables

    return root_flowables


# ------------------------------------------------------------------------------
# 4. Main Document Orchestrator
# ------------------------------------------------------------------------------
def generate_pdf(output_path: str, source_files: List[str],
                 title: str = None,
                 subtitle: str = None,
                 header: str = None,
                 footer: str = None):
    print(f"[PDF Generator] Preparing manual: {output_path}")
    styles = build_styles()
    story = []

    # Title Page Banner
    if title:
        story.append(safe_paragraph(title, styles['ManualTitle']))
    if subtitle:
        story.append(safe_paragraph(subtitle, styles['ManualSubtitle']))
    if title or subtitle:
        story.append(HRFlowable(width="100%", thickness=1.5, color=colors.HexColor("#1E3A8A"), spaceAfter=14))

    for idx, filepath in enumerate(source_files):
        if not os.path.exists(filepath):
            print(f"[Warning] File not found: {filepath}", file=sys.stderr)
            continue

        print(f"[PDF Generator] Ingesting: {filepath}")
        with open(filepath, "r", encoding="utf-8") as f:
            content = f.read()

        if idx > 0:
            story.append(PageBreak())

        base_dir = os.path.dirname(os.path.abspath(filepath))
        flowables = parse_markdown_to_flowables(
            content, styles,
            is_first_doc=(idx == 0),
            has_title_banner=bool(title or subtitle),
            base_dir=base_dir
        )
        story.extend(flowables)

    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
    doc = SimpleDocTemplate(
        output_path,
        pagesize=letter,
        leftMargin=54,
        rightMargin=54,
        topMargin=54,
        bottomMargin=54
    )

    canvas_cls = make_numbered_canvas(
        header_text=header or "ImGee User Guide",
        footer_left=footer or "ImGee for Dear ImGui • Author: Steven L. Goldberg (@slgoldberg)"
    )
    doc.build(story, canvasmaker=canvas_cls)
    file_size_kb = os.path.getsize(output_path) / 1024.0
    print(f"[PDF Generator] Manual created successfully: {output_path} ({file_size_kb:.1f} KB)")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate publication-grade PDF manuals from ImGee Markdown documentation.")
    parser.add_argument("--output", "-o", default=None, help="Output PDF file path")
    parser.add_argument("--imgee", action="store_true", help="Generate dedicated ImGee User Guide PDF")
    parser.add_argument("--manifesto", action="store_true", help="Generate dedicated ImGee UX Design Manifesto PDF")
    parser.add_argument("--all", action="store_true", help="Generate both ImGee User Guide and UX Design Manifesto PDFs")
    parser.add_argument("--title", default=None, help="Manual title")
    parser.add_argument("--subtitle", default=None, help="Manual subtitle")
    parser.add_argument("--header", default=None, help="Running header text")
    parser.add_argument("--footer", default=None, help="Running footer text")
    parser.add_argument("inputs", nargs="*", help="Markdown files to include")

    args = parser.parse_args()

    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    docs_dir = os.path.join(repo_root, "docs")

    imgee_docs = [
        os.path.join(docs_dir, "ImGee_User_Guide.md"),
        os.path.join(repo_root, "extensions", "README.md"),
        os.path.join(repo_root, "extensions", "TimedTooltip", "README.md"),
        os.path.join(repo_root, "extensions", "Markdown", "README.md"),
        os.path.join(repo_root, "extensions", "Typography", "README.md"),
        os.path.join(repo_root, "extensions", "Viewport", "README.md"),
        os.path.join(repo_root, "extensions", "DocViewer", "README.md"),
        os.path.join(repo_root, "widgets", "README.md"),
        os.path.join(repo_root, "themes", "README.md"),
    ]
    imgee_docs = [f for f in imgee_docs if os.path.exists(f)]

    manifesto_doc = os.path.join(docs_dir, "ImGee_UX_Design_Manifesto.md")
    manifesto_docs = [manifesto_doc] if os.path.exists(manifesto_doc) else []

    default_footer = "ImGee for Dear ImGui • Author: Steven L. Goldberg (@slgoldberg)"

    # If explicit inputs are provided, generate single target
    if args.inputs:
        out_path = args.output or os.path.join(docs_dir, "ImGee_User_Guide.pdf")
        is_gallery = any("gallery" in os.path.basename(p).lower() for p in args.inputs)
        generate_pdf(
            out_path,
            args.inputs,
            title=args.title or ("ImGee 2x Retina Widget & Extension Visual Gallery" if is_gallery else "ImGee: Modern Ergonomic UI Toolkit for Dear ImGui"),
            subtitle=args.subtitle or ("Standalone Offscreen 2x Retina Showcase • ImGee v1.1.0" if is_gallery else "Comprehensive Developer & User Manual • ImGee v1.1.0"),
            header=args.header or ("ImGee: Modern Ergonomic UI Toolkit for Dear ImGui -- Visual Gallery" if is_gallery else "ImGee: Modern Ergonomic UI Toolkit for Dear ImGui -- User Guide"),
            footer=args.footer or default_footer
        )
        sys.exit(0)

    # Determine what to build (default with no flags: build both User Guide and UX Manifesto):
    build_imgee = args.imgee or args.all or (not args.manifesto and args.output is None)
    build_manifesto = args.manifesto or args.all or (not args.imgee and args.output is None)

    if args.output:
        is_manifesto = "manifesto" in args.output.lower()
        generate_pdf(
            args.output,
            manifesto_docs if is_manifesto else imgee_docs,
            title=args.title or ("The ImGee UX Design Manifesto" if is_manifesto else "ImGee: Modern Ergonomic UI Toolkit for Dear ImGui"),
            subtitle=args.subtitle or ("Ergonomics, Cognitive Affordances, and Human Factors for User Interfaces • ImGee v1.1.0" if is_manifesto else "Comprehensive Developer & User Manual • ImGee v1.1.0"),
            header=args.header or ("The ImGee UX Design Manifesto -- Platform Standard" if is_manifesto else "ImGee: Modern Ergonomic UI Toolkit for Dear ImGui -- User Guide"),
            footer=args.footer or default_footer
        )
    else:
        if build_imgee and imgee_docs:
            imgee_out = os.path.join(docs_dir, "ImGee_User_Guide.pdf")
            generate_pdf(
                imgee_out,
                imgee_docs,
                title=args.title or "ImGee: Modern Ergonomic UI Toolkit for Dear ImGui",
                subtitle=args.subtitle or "Comprehensive Developer & User Manual • ImGee v1.1.0",
                header=args.header or "ImGee: Modern Ergonomic UI Toolkit for Dear ImGui -- User Guide",
                footer=args.footer or default_footer
            )

        if build_manifesto and manifesto_docs:
            manifesto_out = os.path.join(docs_dir, "ImGee_UX_Design_Manifesto.pdf")
            generate_pdf(
                manifesto_out,
                manifesto_docs,
                title=args.title or "The ImGee UX Design Manifesto",
                subtitle=args.subtitle or "Ergonomics, Cognitive Affordances, and Human Factors for User Interfaces • ImGee v1.1.0",
                header=args.header or "The ImGee UX Design Manifesto -- Platform Standard",
                footer=args.footer or default_footer
            )
