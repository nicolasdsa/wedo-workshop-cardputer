from __future__ import annotations

from fastapi import Depends, Request
from fastapi.responses import HTMLResponse
from sqlalchemy.orm import Session

from core.dependencies import get_db
from core.templates import templates
from schemas.artist import ArtistCreate, ArtistTimelineEntryCreate, ArtistUpdate
from services import artist as artist_service
from services import scenario as scenario_service


def landing_page(request: Request) -> HTMLResponse:
    return templates.TemplateResponse(
        "home.html",
        {"request": request},
    )


def list_artists_page(request: Request, db: Session = Depends(get_db)) -> HTMLResponse:
    query = (request.query_params.get("q") or "").strip()
    artists = artist_service.list_artists_filtered(db, query=query)
    return templates.TemplateResponse(
        "artists/list.html",
        {
            "request": request,
            "artists": artists,
            "query": query,
        },
    )


def list_artists_partial(
    request: Request, params, db: Session = Depends(get_db)
) -> HTMLResponse:
    query = (params.get("q") or "").strip()
    artists = artist_service.list_artists_filtered(db, query=query)
    return templates.TemplateResponse(
        "artists/partials/list.html",
        {
            "request": request,
            "artists": artists,
        },
    )


def new_artist_modal(request: Request) -> HTMLResponse:
    return templates.TemplateResponse(
        "artists/partials/artist_modal.html",
        {
            "request": request,
            "artist": None,
            "modal_title": "Criar artista",
            "submit_label": "Criar",
            "submit_url": "/ui/artists",
        },
    )


def edit_artist_modal(
    artist_id: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    artist = artist_service.get_artist(db, artist_id)
    return templates.TemplateResponse(
        "artists/partials/artist_modal.html",
        {
            "request": request,
            "artist": artist,
            "modal_title": "Editar artista",
            "submit_label": "Salvar",
            "submit_url": f"/ui/artists/{artist_id}",
        },
    )


def delete_artist_modal(
    artist_id: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    artist = artist_service.get_artist(db, artist_id)
    return templates.TemplateResponse(
        "artists/partials/delete_modal.html",
        {
            "request": request,
            "artist": artist,
        },
    )


def _build_timeline_entries(form) -> list[ArtistTimelineEntryCreate]:
    years = form.getlist("timeline_year")
    titles = form.getlist("timeline_title")
    descriptions = form.getlist("timeline_description")
    entries: list[ArtistTimelineEntryCreate] = []
    for idx, year_raw in enumerate(years):
        description = (descriptions[idx] if idx < len(descriptions) else "").strip()
        title = (titles[idx] if idx < len(titles) else "").strip()
        if not year_raw or not description:
            continue
        try:
            year = int(year_raw)
        except ValueError:
            continue
        entries.append(
            ArtistTimelineEntryCreate(year=year, title=title or None, description=description)
        )
    return entries


async def create_artist_action(
    request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    form = await request.form()
    name = (form.get("name") or "").strip()
    bio = (form.get("bio") or "").strip()
    image_path = (form.get("image_path") or "").strip() or None
    timeline = _build_timeline_entries(form)
    payload = ArtistCreate(
        name=name,
        bio=bio,
        image_path=image_path,
        timeline=timeline or None,
    )
    artist_service.create_artist(db, payload)
    return list_artists_partial(request, form, db)


async def update_artist_action(
    artist_id: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    form = await request.form()
    name = (form.get("name") or "").strip()
    bio = (form.get("bio") or "").strip()
    image_path = (form.get("image_path") or "").strip()
    timeline = _build_timeline_entries(form)
    payload = ArtistUpdate(
        name=name or None,
        bio=bio or None,
        image_path=image_path or None,
        timeline=timeline,
    )
    artist_service.update_artist(db, artist_id, payload)
    return list_artists_partial(request, form, db)


async def delete_artist_action(
    artist_id: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    form = await request.form()
    artist_service.delete_artist(db, artist_id)
    return list_artists_partial(request, form, db)


def artist_profile_page(
    artist_id: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    artist = artist_service.get_artist(db, artist_id)
    scenarios = scenario_service.list_scenarios_by_artist(db, artist_id, only_active=True)
    return templates.TemplateResponse(
        "artists/profile.html",
        {
            "request": request,
            "artist": artist,
            "scenarios": scenarios,
        },
    )
