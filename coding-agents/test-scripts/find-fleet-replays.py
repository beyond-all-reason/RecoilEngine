#!/usr/bin/env python3
"""From Adam Burgess's scripts/find-fleet-replays.py in qant-au/tellurion.

Find recent BAR fleet replays recorded on a given engine version.

The public replay API (api.bar-rts.com) cannot filter by engine, so this walks
the newest replays page by page and reads each one's detail record, one request
at a time, until it has found --want matches or scanned --max-pages pages.

usage: find-fleet-replays.py 2026.09.02 [--want 3] [--max-pages 20]
       [--min-minutes 3] [--max-minutes 20]
Prints: id, duration, map, players, download URL.
"""
import argparse
import json
import time
import urllib.parse
import urllib.request

API = "https://api.bar-rts.com"
# The replay site's Download link (bar-rts.com $config.objectStorageUrl + /demos/);
# storage.uk.cdn.beyondallreason.dev stopped resolving by 2026-11-17.
CDN = "https://storage.uk.cloud.ovh.net/v1/AUTH_10286efc0d334efd917d476d7183232e/BAR/demos/"
UA = {"User-Agent": "replay-finder"}


def get(path):
    req = urllib.request.Request(API + path, headers=UA)
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.load(r)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("engine")
    ap.add_argument("--want", type=int, default=3)
    ap.add_argument("--max-pages", type=int, default=20)
    ap.add_argument("--min-minutes", type=float, default=3)
    ap.add_argument("--max-minutes", type=float, default=20)
    a = ap.parse_args()

    found, scanned = 0, 0
    for page in range(1, a.max_pages + 1):
        rows = get(f"/replays?page={page}&limit=24")["data"]
        if not rows:
            break
        for row in rows:
            minutes = (row.get("durationMs") or 0) / 60000
            if not (a.min_minutes <= minutes <= a.max_minutes):
                continue
            scanned += 1
            d = get("/replays/" + row["id"])
            time.sleep(0.2)
            if d.get("engineVersion") != a.engine:
                continue
            players = " v ".join(str(len(at.get("Players", [])) + len(at.get("AIs", []))) for at in d.get("AllyTeams", []))
            url = CDN + urllib.parse.quote(d["fileName"])
            print(f"{row['id']}\t{minutes:.1f} min\t{row['Map'].get('scriptName')}\t{players}\t{d.get('gameVersion')}\t{url}")
            found += 1
            if found >= a.want:
                print(f"# scanned {scanned} replay details over {page} page(s)")
                return
    print(f"# found {found}; scanned {scanned} replay details over {a.max_pages} page(s)")


if __name__ == "__main__":
    main()
