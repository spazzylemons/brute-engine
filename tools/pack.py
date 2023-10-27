"""
Classes for building a pack file for the Brute engine to read.
"""

_FLAG_FOLDER = 0x80000000

import struct

# A pack node.
class Node:
    def __init__(self, name: str):
        self.name = name

    @property
    def name(self) -> str:
        return self._name.decode()

    @name.setter
    def name(self, value: str):
        newname = value.encode()
        if len(newname) > 8:
            raise ValueError(f'Node name {repr(value)} too long')
        self._name = newname

# A branch node.
class Branch(Node):
    def __init__(self, name: str):
        super().__init__(name)
        self._children = []

    @property
    def children(self) -> list[Node]:
        return self._children

    # Serialize a branch as a pack file with itself as the root.
    def serialize(self) -> bytes:
        all_entries = []
        lumps = bytearray()

        def walk(branch: Branch):
            folders = []
            for entry in branch.children:
                if isinstance(entry, Branch):
                    offset = 0
                    folders.append(len(all_entries))
                    all_entries.append(entry)
                else:
                    offset = len(lumps)
                    lumps.extend(entry.content)
                    all_entries.append((len(entry.content), offset, entry.name.encode()))
            for folder in folders:
                entry = all_entries[folder]
                offset = len(all_entries)
                size = walk(entry)
                all_entries[folder] = (_FLAG_FOLDER | size, offset, entry.name.encode())
            return len(branch.children)

        all_entries.append(None)
        root_size = walk(self)
        all_entries[0] = (_FLAG_FOLDER | root_size, 1, b'PACK' + struct.pack('<I', len(all_entries)))
        contents = bytearray()
        for entry in all_entries:
            size, offset, name = entry
            if not (size & _FLAG_FOLDER):
                offset += len(all_entries * 16)
            contents.extend(name.ljust(8, b'\0'))
            contents.extend(struct.pack('<II', size, offset))
        contents.extend(lumps)
        return bytes(contents)

# A lump node.
class Lump(Node):
    def __init__(self, name: str, content: bytes):
        super().__init__(name)
        self.content = content
