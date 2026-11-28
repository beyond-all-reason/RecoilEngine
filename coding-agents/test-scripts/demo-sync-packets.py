#!/usr/bin/env python3
"""From Adam Burgess's scripts/demo-sync-packets.py in qant-au/tellurion.

Count the sync checksums recorded in a demo (.sdfz).

The engine compares its own checksums only against the ones a demo carries,
and logs nothing when they match. A demo with none would replay "clean"
whatever the simulation did, so replay-check.sh refuses one, and
requires playback to reach the last frame that has a checksum.

Prints: <number of NETMSG_SYNCRESPONSE packets> <last frame that has one>
"""
import gzip
import struct
import sys

NETMSG_SYNCRESPONSE = 33  # rts/Net/Protocol/NetMessageTypes.h

data = gzip.open(sys.argv[1]).read()
# DemoFileHeader (rts/System/LoadSave/demofile.h): magic[16], version,
# headerSize, versionString[256], gameID[16], unixTime, scriptSize, demoStreamSize
header_size = struct.unpack_from("<i", data, 20)[0]
script_size, stream_size = struct.unpack_from("<ii", data, 16 + 4 + 4 + 256 + 16 + 8)
pos = header_size + script_size
end = min(pos + stream_size, len(data))
count = 0
last_frame = 0
# the stream is a run of chunks: float game time, uint32 length, then one packet
while pos + 8 <= end:
    length = struct.unpack_from("<I", data, pos + 4)[0]
    pos += 8
    # packet: id, player, int32 frame, uint32 checksum
    if length >= 10 and data[pos] == NETMSG_SYNCRESPONSE:
        count += 1
        last_frame = max(last_frame, struct.unpack_from("<i", data, pos + 2)[0])
    pos += length
print(count, last_frame)
