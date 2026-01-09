from __future__ import annotations

from sqlalchemy.orm import Session

from core.exceptions import ConflictError, NotFoundError
from models.artist import Artist
from models.artist_timeline_entry import ArtistTimelineEntry
from schemas.artist import ArtistCreate, ArtistUpdate


def create_artist(db: Session, data: ArtistCreate) -> Artist:
    existing = db.query(Artist).filter(Artist.name == data.name).first()
    if existing:
        raise ConflictError("Artista já cadastrado.")

    artist = Artist(
        name=data.name,
        image_path=data.image_path,
        bio=data.bio,
    )
    db.add(artist)
    db.flush()

    if data.timeline:
        for entry in data.timeline:
            artist.timeline_entries.append(
                ArtistTimelineEntry(
                    artist_id=artist.id,
                    year=entry.year,
                    title=entry.title,
                    description=entry.description,
                )
            )

    db.commit()
    db.refresh(artist)
    return artist


def get_artist(db: Session, artist_id: int) -> Artist:
    artist = db.get(Artist, artist_id)
    if not artist:
        raise NotFoundError("Artista não encontrado.")
    return artist


def list_artists(db: Session) -> list[Artist]:
    return db.query(Artist).order_by(Artist.name).all()


def list_artists_filtered(db: Session, query: str | None = None) -> list[Artist]:
    artists_query = db.query(Artist)
    if query:
        artists_query = artists_query.filter(Artist.name.ilike(f"%{query}%"))
    return artists_query.order_by(Artist.name).all()


def update_artist(db: Session, artist_id: int, data: ArtistUpdate) -> Artist:
    artist = get_artist(db, artist_id)

    if data.name and data.name != artist.name:
        existing = db.query(Artist).filter(Artist.name == data.name).first()
        if existing and existing.id != artist.id:
            raise ConflictError("Artista já cadastrado.")
        artist.name = data.name

    if data.image_path is not None:
        artist.image_path = data.image_path
    if data.bio is not None:
        artist.bio = data.bio

    if data.timeline is not None:
        artist.timeline_entries.clear()
        for entry in data.timeline:
            artist.timeline_entries.append(
                ArtistTimelineEntry(
                    artist_id=artist.id,
                    year=entry.year,
                    title=entry.title,
                    description=entry.description,
                )
            )

    db.add(artist)
    db.commit()
    db.refresh(artist)
    return artist


def delete_artist(db: Session, artist_id: int) -> None:
    artist = get_artist(db, artist_id)
    db.delete(artist)
    db.commit()
