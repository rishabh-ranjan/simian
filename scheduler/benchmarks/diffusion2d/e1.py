#!/usr/bin/env python3
import matplotlib.pyplot as plt

f = open('e1.tsv')
header = f.readline().strip().split('\t')
stats = []
for l in f.readlines():
    v = l.strip().split('\t')
    d = {}
    for i, k in enumerate(header):
        d[k] = v[i]
    stats.append(d)

stats = sorted(stats, key=lambda d: int(d['nprocs']))

nprocs = [int(d['nprocs']) for d in stats]
total_time = [int(d['total time']) for d in stats]
scheduler_time = [int(d['scheduler time']) for d in stats]
processing_time = [int(d['processing time']) for d in stats]
solver_time = [int(d['solver time']) for d in stats]
misc_time = [int(d['misc time']) for d in stats]

plt.figure()
plt.plot(nprocs, total_time, 'd-', label='total')
plt.plot(nprocs, scheduler_time, 'd-', label='scheduler')
plt.plot(nprocs, processing_time, 'd-', label='processing')
plt.plot(nprocs, solver_time, 'd-', label='solver')
plt.plot(nprocs, misc_time, 'd-', label='misc')
plt.legend()
plt.xlabel('#processes')
plt.ylabel('time(ms)')
plt.title('Runtime Analysis')
plt.savefig('time_nprocs.png')

plt.figure()
total_generators = [int(d['total generators']) for d in stats]
plt.plot(nprocs, total_generators, 'd-')
plt.xlabel('#processes')
plt.ylabel('#generators')
plt.title('Symmetry Analysis')
plt.savefig('sym_nprocs.png')

plt.figure()
total_epochs = [int(d['total epochs']) for d in stats]
unique_epochs = [int(d['unique epochs']) for d in stats]
repeated_epochs = [int(d['repeated epochs']) for d in stats]
plt.plot(nprocs, total_epochs, 'd-', label='total')
plt.plot(nprocs, unique_epochs, 'd-', label='unique')
plt.plot(nprocs, repeated_epochs, 'd-', label='repeated')
plt.legend()
plt.xlabel('#processes')
plt.ylabel('#epochs')
plt.title('Epoch Analysis')
plt.savefig('epoch_nprocs.png')
