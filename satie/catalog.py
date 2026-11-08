# SPDX-License-Identifier: GPL-3.0-only
# Suggested electronic arrangement tempos, not historical performance markings.
SATIE = []
for series, group, keys, tempos in [
    ('gymnopedie', 'Gymnopedies', [(2,0),(0,0),(9,5)], [60,56,58]),
    ('gnossienne', 'Gnossiennes', [(5,5),(9,5),(9,5)], [64,60,58]),
    ('ogive', 'Ogives', [(0,0),(0,0),(0,0),(0,0)], [60,56,56,58]),
]:
    for i, ((root, mode), tempo) in enumerate(zip(keys, tempos), 1):
        SATIE.append(dict(file=f'{series}_{i}', title=f'{series.upper()} {i}',
            group=group, composer='Erik Satie', root=root, mode=mode,
            tempo=tempo, slow=True))
