"""Render sprite sheets over a white page so they can be judged by eye."""
from PIL import Image


def sheet_image(canvas, zoom=4, columns=6, pad=4):
    """Composite a sprite sheet onto white and upscale it losslessly."""
    frame = canvas.to_image()
    white = Image.new("RGBA", frame.size, (255, 255, 255, 255))
    flat = Image.alpha_composite(white, frame).convert("L")
    rows = (len(canvas.pixels) // (canvas.width * columns) or 1)
    cell_w = canvas.width // columns
    cell_h = canvas.height // rows
    out = Image.new("L", (columns * (cell_w * zoom + pad) + pad,
                          rows * (cell_h * zoom + pad) + pad), 255)
    for index in range(rows * columns):
        col, row = index % columns, index // columns
        cell = flat.crop((col * cell_w, row * cell_h,
                          (col + 1) * cell_w, (row + 1) * cell_h))
        cell = cell.resize((cell_w * zoom, cell_h * zoom), Image.NEAREST)
        out.paste(cell, (pad + col * (cell_w * zoom + pad),
                         pad + row * (cell_h * zoom + pad)))
    return out
