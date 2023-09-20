import argparse
import subprocess
import time
import random
import shlex

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1


def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server


def run(command, output=None):
    process = subprocess.Popen(shlex.split(
        command), stdout=output, stderr=subprocess.DEVNULL)
    return process


def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()


def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')


def generate_flamegraph():
    command = 'sudo perf script -i perf.data | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl > graph.svg'
    double_pipe = subprocess.Popen(command, shell=True)
    double_pipe.wait()

server = run(start_server())
time.sleep(1)
# запускал процесс perf record с возможностью записи трассировки функций для процесса сервера,
perf_record = run('perf record -g -o perf.data -p ' + str(server.pid))
make_shots()
# корректно завершал работу perf record, так чтобы на выходе получался непустой и неповрежденный файл perf.data
time.sleep(1) 
stop(perf_record, False)
time.sleep(1) 
# запускал двойной пайп для построения флеймграфа, так чтобы на выходе получался файл graph.svg, содержащий флеймграф работы сервера в процессе обстрела его запросами,
stop(server)
time.sleep(1)
# flame = run('sudo perf script -i perf.data | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl > graph.svg')
generate_flamegraph()
time.sleep(2)
# stop(flame, True)

print('Job done')
