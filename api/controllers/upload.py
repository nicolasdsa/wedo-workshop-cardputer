from __future__ import annotations

import os
import secrets
from pathlib import Path

from fastapi import File, HTTPException, UploadFile
from fastapi.responses import JSONResponse


ALLOWED_PREFIXES = ("image/",)
UPLOAD_DIR = Path(__file__).resolve().parent.parent / "static" / "uploads"


def upload_file(file: UploadFile = File(...)) -> JSONResponse:
    if not file.content_type or not file.content_type.startswith(ALLOWED_PREFIXES):
        raise HTTPException(status_code=400, detail="Tipo de arquivo não suportado. Envie uma imagem ou GIF.")

    UPLOAD_DIR.mkdir(parents=True, exist_ok=True)
    suffix = Path(file.filename or "").suffix
    filename = f"{secrets.token_hex(8)}{suffix}"
    dest_path = UPLOAD_DIR / filename

    with dest_path.open("wb") as buffer:
        buffer.write(file.file.read())

    response_payload: dict[str, str] = {"path": f"/static/uploads/{filename}"}

    is_gif = (file.content_type == "image/gif") or (suffix.lower() == ".gif")
    if is_gif:
        try:
            from PIL import Image
        except Exception:
            Image = None
        if Image:
            try:
                frame_path = dest_path.with_name(f"{dest_path.stem}_frame0.png")
                with Image.open(dest_path) as img:
                    img.seek(0)
                    img.convert("RGBA").save(frame_path)
                response_payload["first_frame_path"] = f"/static/uploads/{frame_path.name}"
            except Exception:
                # Fail silently to keep GIF upload working without frame extraction.
                pass

    return JSONResponse(response_payload)
