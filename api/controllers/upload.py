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

    relative_path = f"uploads/{filename}"
    return JSONResponse({"path": relative_path})
