#!/usr/bin/env python3
# ==============================================================================
# ImgWindow User Manual PDF Generator (tools/generate_manual.py)
# ------------------------------------------------------------------------------
# Author: Steven L. Goldberg (@slgoldberg)
# Adapted from Sir Lance's ReportLab manual generator pattern for FlyWithLua.
#
# Ingests contrib/ Markdown documentation and compiles a clean, publication-grade
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
        SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, KeepTogether, HRFlowable, Preformatted, Image as RLImage
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
HAS_BOX_DRAWING = False
HAS_APPLE_SYMBOLS = False
HAS_ARIAL_UNICODE = False

def init_pdf_fonts():
    global MONO_FONT, MONO_BOLD_FONT, HAS_BOX_DRAWING
    global HAS_APPLE_SYMBOLS, HAS_ARIAL_UNICODE

    # 1. Monospace discovery
    mono_candidates = [
        ('/System/Library/Fonts/Supplemental/Courier New.ttf', '/System/Library/Fonts/Supplemental/Courier New Bold.ttf'),
        ('/Library/Fonts/Courier New.ttf', '/Library/Fonts/Courier New Bold.ttf'),
        ('/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf', '/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf'),
        ('/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf', '/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf'),
        ('C:\\Windows\\Fonts\\cour.ttf', 'C:\\Windows\\Fonts\\courbd.ttf'),
    ]
    for reg_path, bold_path in mono_candidates:
        if os.path.exists(reg_path):
            try:
                pdfmetrics.registerFont(TTFont('MonoTTF', reg_path))
                MONO_FONT = 'MonoTTF'
                HAS_BOX_DRAWING = True
                if os.path.exists(bold_path):
                    pdfmetrics.registerFont(TTFont('MonoTTFBold', bold_path))
                    MONO_BOLD_FONT = 'MonoTTFBold'
                else:
                    MONO_BOLD_FONT = 'MonoTTF'
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
def make_numbered_canvas(header_text: str = "ImGee: Avionics & Flight-Sim UI Toolkit — User Guide",
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
        code_spans.append(f'<font name="{MONO_BOLD_FONT}" color="#B91C1C"><b>{code_content}</b></font>')
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

    # 9. Transform or sanitize custom tags (<badge>, <pill>, <kbd>, <key>, <kdb>)
    text = re.sub(
        r'<(?:key|kbd|kdb)(?:\s+[^>]*)?>(.+?)</(?:key|kbd|kdb)>',
        r'<font name="Helvetica-Bold" color="#0F172A"><b>[&nbsp;\1&nbsp;]</b></font>',
        text,
        flags=re.IGNORECASE,
    )
    valid_tags = {'b', '/b', 'i', '/i', 'u', '/u', 'font', '/font', 'link', '/link', 'a', '/a'}
    def tag_filter(m):
        raw = m.group(0)
        tag_name = m.group(1).split()[0].lower()
        if tag_name in valid_tags or tag_name.startswith('font ') or tag_name.startswith('link '):
            return raw
        if any(tag_name.startswith(k) for k in ('badge', 'pill', 'kbd', 'tag', 'btn', 'key', 'code', 'mono')):
            return f'<font name="Helvetica-Bold" color="#1E293B">[{raw[1:-1]}]</font>'
        return raw.replace("<", "&lt;").replace(">", "&gt;")

    text = re.sub(r'</?([a-zA-Z0-9_\-=:#, ]+)>', tag_filter, text)

    # 10. Escape any remaining stray angle brackets
    text = re.sub(r'<(?!(?:b|/b|i|/i|u|/u|font|/font|link|/link|a|/a)(?:\s|>))', '&lt;', text)

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
    try:
        return Paragraph(formatted, style)
    except Exception:
        plain = re.sub(r'<[^>]+>', '', raw_text)
        plain = plain.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        return Paragraph(plain, style)


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
        fontSize=7.2,
        leading=9.2,
        textColor=colors.HexColor("#F8FAFC")
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
    flowables = []
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

    def flush_table(preset_cols=None):
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
        if preset_cols and len(preset_cols) == num_cols and sum(preset_cols) > 0:
            total_p = sum(preset_cols)
            col_widths = [CONTENT_WIDTH * (w / total_p) for w in preset_cols]
        else:
            col_width = CONTENT_WIDTH / num_cols if num_cols > 0 else CONTENT_WIDTH
            col_widths = [col_width] * num_cols

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
                    # Chunk code lines into rows of max 20 lines so multi-page code blocks break cleanly
                    CHUNK_SIZE = 20
                    table_data = []
                    for chunk_idx in range(0, len(code_lines), CHUNK_SIZE):
                        chunk_slice = code_lines[chunk_idx:chunk_idx + CHUNK_SIZE]
                        raw_code = "\n".join(chunk_slice).replace("\t", "    ")
                        if not HAS_BOX_DRAWING:
                            raw_code = (raw_code
                                .replace("├──", "|--")
                                .replace("└──", "\\--")
                                .replace("│", "|")
                                .replace("─", "-")
                            )

                        cell_elements = []
                        if chunk_idx == 0 and code_lang:
                            chip_style = ParagraphStyle(
                                'LangChip',
                                fontName='Helvetica-Bold',
                                fontSize=7.0,
                                textColor=colors.HexColor("#94A3B8"),
                                spaceAfter=2
                            )
                            cell_elements.append(Paragraph(f"<b>[{code_lang.upper()}]</b>", chip_style))
                        cell_elements.append(Preformatted(raw_code, styles['CodeSnippet']))
                        table_data.append([cell_elements])

                    if not table_data:
                        table_data = [[[Preformatted("", styles['CodeSnippet'])]]]

                    tbl = Table(table_data, colWidths=[CONTENT_WIDTH])
                    tbl.setStyle(TableStyle([
                        ('BACKGROUND', (0, 0), (-1, -1), colors.HexColor("#0F172A")),
                        ('BOX', (0, 0), (-1, -1), 0.5, colors.HexColor("#334155")),
                        ('LEFTPADDING', (0, 0), (-1, -1), 8),
                        ('RIGHTPADDING', (0, 0), (-1, -1), 8),
                        ('TOPPADDING', (0, 0), (-1, -1), 3),
                        ('BOTTOMPADDING', (0, 0), (-1, -1), 3),
                    ]))
                    flowables.append(Spacer(1, 4))
                    flowables.append(tbl)
                    flowables.append(Spacer(1, 6))

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
            preset_cols = None
            if re.match(r'^\s*<!--\s*table(?:-preset|-state)?:.*?-->\s*$', line, re.IGNORECASE):
                m_cols = re.search(r'cols\s*=\s*["\']?([0-9.,;\s]+)["\']?', line, re.IGNORECASE)
                if m_cols:
                    try:
                        raw_nums = re.findall(r'[0-9]+(?:\.[0-9]+)?', m_cols.group(1))
                        preset_cols = [float(x) for x in raw_nums]
                    except Exception:
                        preset_cols = None
                i += 1
            flush_table(preset_cols)
            if preset_cols is not None:
                continue

        # Ignored HTML comments (e.g. metadata, annotations)
        if re.match(r'^\s*<!--.*?-->\s*$', line):
            i += 1
            continue

        if not line.strip():
            i += 1
            continue

        # Page Breaks (<!-- pagebreak -->, <!-- newpage -->, \newpage, \pagebreak, <pagebreak />, or --- <newpage ...>)
        if re.match(r'^\s*(?:<!--\s*(?:pagebreak|newpage)\s*-->|\\pagebreak|\\newpage|<pagebreak\s*/?>|[-*_]{3,}\s*<new\s*page.*?>)\s*$', line, re.IGNORECASE):
            flowables.append(PageBreak())
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

        # Centered block wrapper tags (<center>, </center>, <centre>, </centre>)
        if re.match(r'^\s*</?cent(?:er|re)>\s*$', line, re.IGNORECASE):
            i += 1
            continue

        # Markdown Images: ![Caption](path/to/image.png), ![Caption](<path/to/image.png =50%>), or trailing { width=... }
        clean_img_line = re.sub(r'</?cent(?:er|re)>', '', line, flags=re.IGNORECASE).strip()
        img_match = re.match(r'^\s*!\[([^\]]*)\]\(([^)]+)\)(?:\{([^}]*)\})?\s*$', clean_img_line)
        if img_match:
            if in_alert: flush_alert()
            if in_table: flush_table()
            caption_text = img_match.group(1).strip()
            img_target = img_match.group(2).strip()
            attr_block = (img_match.group(3) or "").strip()

            # Unwrap optional CommonMark angle brackets: e.g. "<images/pic.png =50%>" or "<images/pic.png> =50%"
            extra_tokens = []
            if img_target.startswith('<') and '>' in img_target:
                gt_idx = img_target.find('>')
                inner_target = img_target[1:gt_idx].strip()
                after_gt = img_target[gt_idx + 1:].strip()
                if after_gt:
                    extra_tokens.extend(after_gt.split())
                img_target = inner_target

            raw_parts = img_target.split()
            tail_mods = []
            while len(raw_parts) > 1 and (
                raw_parts[-1].startswith('=')
                or raw_parts[-1].lower().startswith(('width=', 'w=', 'height=', 'h='))
            ):
                tail_mods.insert(0, raw_parts.pop())
            img_rel_path = " ".join(raw_parts) if raw_parts else ""
            mod_tokens = tail_mods + extra_tokens + (attr_block.split() if attr_block else [])

            pct_mod = None
            px_w_mod = None
            px_h_mod = None
            for tok in mod_tokens:
                t = tok.strip()
                if t.startswith('='):
                    dim = t[1:].strip()
                    if dim.endswith('%'):
                        try:
                            pct_mod = float(dim[:-1]) / 100.0
                        except ValueError:
                            pass
                    elif 'x' in dim.lower():
                        sw, sh = dim.lower().split('x', 1)
                        try:
                            if sw: px_w_mod = float(sw)
                            if sh: px_h_mod = float(sh)
                        except ValueError:
                            pass
                    else:
                        try:
                            px_w_mod = float(dim)
                        except ValueError:
                            pass
                elif t.lower().startswith(('width=', 'w=')):
                    val = t.split('=', 1)[1].strip('"\'')
                    if val.endswith('%'):
                        try:
                            pct_mod = float(val[:-1]) / 100.0
                        except ValueError:
                            pass
                    else:
                        val = re.sub(r'px$', '', val, flags=re.IGNORECASE)
                        try:
                            px_w_mod = float(val)
                        except ValueError:
                            pass
                elif t.lower().startswith(('height=', 'h=')):
                    val = t.split('=', 1)[1].strip('"\'')
                    val = re.sub(r'px$', '', val, flags=re.IGNORECASE)
                    try:
                        px_h_mod = float(val)
                    except ValueError:
                        pass

            if base_dir and not os.path.isabs(img_rel_path):
                img_full_path = os.path.normpath(os.path.join(base_dir, img_rel_path))
            else:
                img_full_path = img_rel_path

            if os.path.exists(img_full_path):
                try:
                    with PILImage.open(img_full_path) as pimg:
                        orig_w, orig_h = pimg.size

                    max_w = CONTENT_WIDTH - 20
                    aspect = (float(orig_h) / float(orig_w)) if orig_w > 0 else 1.0
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
                        cell_elements.append(Paragraph(f"<i>{caption_text}</i>", styles['FigCaption']))

                    fig_tbl = Table([[cell_elements]], colWidths=[target_w])
                    fig_tbl.setStyle(TableStyle([
                        ('BOX', (0, 0), (-1, -1), 0.75, colors.HexColor("#334155")),
                        ('BACKGROUND', (0, 0), (-1, -1), colors.HexColor("#0F172A")),
                        ('ALIGN', (0, 0), (-1, -1), 'CENTER'),
                        ('TOPPADDING', (0, 0), (-1, -1), 2),
                        ('BOTTOMPADDING', (0, 0), (-1, -1), 3),
                        ('LEFTPADDING', (0, 0), (-1, -1), 2),
                        ('RIGHTPADDING', (0, 0), (-1, -1), 2),
                    ]))

                    flowables.append(Spacer(1, 4))
                    flowables.append(KeepTogether(fig_tbl))
                    flowables.append(Spacer(1, 6))
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

    return flowables


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
        story.append(Paragraph(title, styles['ManualTitle']))
    if subtitle:
        story.append(Paragraph(subtitle, styles['ManualSubtitle']))
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
    import shutil
    parser = argparse.ArgumentParser(description="Generate publication-grade PDF manual from ImgWindow / ImGee markdown documentation.")
    parser.add_argument("--output", "-o", default=None, help="Output PDF file path")
    parser.add_argument("--imgee", action="store_true", help="Generate dedicated ImGee User Guide PDF")
    parser.add_argument("--manifesto", action="store_true", help="Generate dedicated ImGee UX Design Manifesto PDF")
    parser.add_argument("--contrib", action="store_true", help="Generate full ImgWindow Contrib Library manual")
    parser.add_argument("--all", action="store_true", help="Generate all documentation manuals and companions")
    parser.add_argument("--title", default=None, help="Manual title")
    parser.add_argument("--subtitle", default=None, help="Manual subtitle")
    parser.add_argument("--header", default=None, help="Running header text")
    parser.add_argument("--footer", default=None, help="Running footer text")
    parser.add_argument("inputs", nargs="*", help="Markdown files to include")

    args = parser.parse_args()

    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    imgee_docs = [
        os.path.join(repo_root, "contrib", "ImGee", "docs", "ImGee_User_Guide.md"),
        os.path.join(repo_root, "contrib", "ImGee", "extensions", "README.md"),
        os.path.join(repo_root, "contrib", "ImGee", "extensions", "TimedTooltip", "README.md"),
        os.path.join(repo_root, "contrib", "ImGee", "extensions", "Markdown", "README.md"),
        os.path.join(repo_root, "contrib", "ImGee", "extensions", "Typography", "README.md"),
        os.path.join(repo_root, "contrib", "ImGee", "widgets", "README.md"),
    ]
    imgee_docs = [f for f in imgee_docs if os.path.exists(f)]

    contrib_docs = [
        os.path.join(repo_root, "contrib", "README.md"),
        os.path.join(repo_root, "contrib", "ImGee", "docs", "ImGee_User_Guide.md"),
        os.path.join(repo_root, "contrib", "ImGee", "docs", "ImGee_UX_Design_Manifesto.md"),
        os.path.join(repo_root, "contrib", "ImGee", "extensions", "README.md"),
        os.path.join(repo_root, "contrib", "ImGee", "extensions", "TimedTooltip", "README.md"),
        os.path.join(repo_root, "contrib", "ImGee", "extensions", "Markdown", "README.md"),
        os.path.join(repo_root, "contrib", "ImGee", "extensions", "Typography", "README.md"),
        os.path.join(repo_root, "contrib", "ImGee", "widgets", "README.md"),
    ]
    contrib_docs = [f for f in contrib_docs if os.path.exists(f)]

    manifesto_doc = os.path.join(repo_root, "contrib", "ImGee", "docs", "ImGee_UX_Design_Manifesto.md")
    manifesto_docs = [manifesto_doc] if os.path.exists(manifesto_doc) else []

    # If explicit inputs are provided, generate single target
    if args.inputs:
        out_path = args.output or os.path.join(repo_root, "contrib", "ImGee", "docs", "ImGee_User_Guide.pdf")
        generate_pdf(
            out_path,
            args.inputs,
            title=args.title or "ImGee: Avionics & Flight-Sim UI Toolkit",
            subtitle=args.subtitle or "Comprehensive Developer & User Manual • ImGee v1.0.0",
            header=args.header or "ImGee: Avionics & Flight-Sim UI Toolkit — User Guide",
            footer=args.footer or "ImGee for Dear ImGui • Author: Steven L. Goldberg (@slgoldberg) and the Rogers"
        )
        sys.exit(0)

    # Determine what to build:
    build_imgee = args.imgee or (not args.contrib and not args.manifesto and args.output is None)
    build_manifesto = args.manifesto or args.all or (not args.contrib and args.output is None)
    build_contrib = args.contrib or args.all

    if args.output:
        # User specified an output file
        is_imgee = "imgee" in args.output.lower()
        generate_pdf(
            args.output,
            imgee_docs if is_imgee else contrib_docs,
            title=args.title or ("ImGee: Avionics & Flight-Sim UI Toolkit" if is_imgee else "ImgWindow Contrib Library & ImGee Suite"),
            subtitle=args.subtitle or ("Comprehensive Developer & User Manual • ImGee v1.0.0" if is_imgee else "Community Multi-Package Container & Avionics UI System for X-Plane Plugins"),
            header=args.header or ("ImGee: Avionics & Flight-Sim UI Toolkit — User Guide" if is_imgee else "ImgWindow Contrib Library & ImGee Suite — Developer Manual"),
            footer=args.footer or ("ImGee for Dear ImGui • Author: Steven L. Goldberg (@slgoldberg) and the Rogers" if is_imgee else "ImgWindow for X-Plane • Author: Steven L. Goldberg (@slgoldberg)")
        )
    else:
        docs_dir = os.path.join(repo_root, "docs")
        imgee_docs_dir = os.path.join(repo_root, "contrib", "ImGee", "docs")

        if build_imgee:
            imgee_out = os.path.join(repo_root, "contrib", "ImGee", "docs", "ImGee_User_Guide.pdf")
            generate_pdf(
                imgee_out,
                imgee_docs,
                title=args.title or "ImGee: Avionics & Flight-Sim UI Toolkit",
                subtitle=args.subtitle or "Comprehensive Developer & User Manual • ImGee v1.0.0",
                header=args.header or "ImGee: Avionics & Flight-Sim UI Toolkit — User Guide",
                footer=args.footer or "ImGee for Dear ImGui • Author: Steven L. Goldberg (@slgoldberg) and the Rogers"
            )

            # Manage symlinks from ImgWindow/docs/ to contrib/ImGee/docs/
            # 1. Symlink: docs/ImGee -> ../contrib/ImGee/docs
            imgee_dir_link = os.path.join(docs_dir, "ImGee")
            if os.path.islink(imgee_dir_link):
                os.remove(imgee_dir_link)
            elif os.path.isdir(imgee_dir_link):
                shutil.rmtree(imgee_dir_link)
            os.symlink("../contrib/ImGee/docs", imgee_dir_link)
            print(f"[PDF Generator] Linked: {imgee_dir_link} -> ../contrib/ImGee/docs")

            # 2. Symlink: docs/ImGee_User_Guide.pdf -> ../contrib/ImGee/docs/ImGee_User_Guide.pdf
            pdf_link = os.path.join(docs_dir, "ImGee_User_Guide.pdf")
            if os.path.exists(pdf_link) or os.path.islink(pdf_link):
                os.remove(pdf_link)
            os.symlink("../contrib/ImGee/docs/ImGee_User_Guide.pdf", pdf_link)
            print(f"[PDF Generator] Linked: {pdf_link} -> ../contrib/ImGee/docs/ImGee_User_Guide.pdf")

            # 3. Symlink: docs/ImGee_User_Guide.md -> ../contrib/ImGee/docs/ImGee_User_Guide.md
            md_link = os.path.join(docs_dir, "ImGee_User_Guide.md")
            if os.path.exists(md_link) or os.path.islink(md_link):
                os.remove(md_link)
            os.symlink("../contrib/ImGee/docs/ImGee_User_Guide.md", md_link)
            print(f"[PDF Generator] Linked: {md_link} -> ../contrib/ImGee/docs/ImGee_User_Guide.md")

        if build_manifesto and manifesto_docs:
            manifesto_out = os.path.join(repo_root, "contrib", "ImGee", "docs", "ImGee_UX_Design_Manifesto.pdf")
            generate_pdf(
                manifesto_out,
                manifesto_docs,
                title="The ImGee UX Design Manifesto",
                subtitle="Ergonomics, Cognitive Affordances, and Human Factors for Simulation User Interfaces",
                header="The ImGee UX Design Manifesto — Platform Standard",
                footer="ImGee Platform Standard • Author: Steven L. Goldberg (@slgoldberg) & Antigravity Team"
            )

            # 4. Symlink: docs/ImGee_UX_Design_Manifesto.pdf -> ../contrib/ImGee/docs/ImGee_UX_Design_Manifesto.pdf
            manifesto_pdf_link = os.path.join(docs_dir, "ImGee_UX_Design_Manifesto.pdf")
            if os.path.exists(manifesto_pdf_link) or os.path.islink(manifesto_pdf_link):
                os.remove(manifesto_pdf_link)
            os.symlink("../contrib/ImGee/docs/ImGee_UX_Design_Manifesto.pdf", manifesto_pdf_link)
            print(f"[PDF Generator] Linked: {manifesto_pdf_link} -> ../contrib/ImGee/docs/ImGee_UX_Design_Manifesto.pdf")

            # 5. Symlink: docs/ImGee_UX_Design_Manifesto.md -> ../contrib/ImGee/docs/ImGee_UX_Design_Manifesto.md
            manifesto_md_link = os.path.join(docs_dir, "ImGee_UX_Design_Manifesto.md")
            if os.path.exists(manifesto_md_link) or os.path.islink(manifesto_md_link):
                os.remove(manifesto_md_link)
            os.symlink("../contrib/ImGee/docs/ImGee_UX_Design_Manifesto.md", manifesto_md_link)
            print(f"[PDF Generator] Linked: {manifesto_md_link} -> ../contrib/ImGee/docs/ImGee_UX_Design_Manifesto.md")

        if build_contrib:
            contrib_out = os.path.join(repo_root, "docs", "ImgWindow_Contrib_Manual.pdf")
            generate_pdf(
                contrib_out,
                contrib_docs,
                title=args.title or "ImgWindow Contrib Library & ImGee Suite",
                subtitle=args.subtitle or "Community Multi-Package Container & Avionics UI System for X-Plane Plugins",
                header=args.header or "ImgWindow Contrib Library & ImGee Suite — Developer Manual",
                footer=args.footer or "ImgWindow for X-Plane • Author: Steven L. Goldberg (@slgoldberg)"
            )
