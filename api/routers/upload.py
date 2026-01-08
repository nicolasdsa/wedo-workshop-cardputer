from fastapi import APIRouter, UploadFile, File
from fastapi.responses import JSONResponse

from controllers import upload

router = APIRouter(
    prefix="/uploads",
    tags=["Uploads"],
    default_response_class=JSONResponse,
)


@router.post("/", response_class=JSONResponse)
async def upload_file_route(file: UploadFile = File(...)):
    return upload.upload_file(file)
