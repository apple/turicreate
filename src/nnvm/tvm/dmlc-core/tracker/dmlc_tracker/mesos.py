#!/usr/bin/env python
"""
DMLC submission script by mesos

One need to make sure all slaves machines are ssh-able.
"""
from __future__ import absolute_import

import os
import sys
import json
import uuid
import logging
from threading import Thread
from . import tracker
try:
    import pymesos.subprocess
    logging.getLogger('pymesos').setLevel(logging.WARNING)

    def _run(prog, env, resources):
        cwd = os.getcwd()
        pymesos.subprocess.check_call(
            prog, shell=True, env=env, cwd=cwd,
            cpus=resources['cpus'], mem=resources['mem']
        )

    _USE_PYMESOS = True

except ImportError:
    import subprocess
    DEVNULL = open(os.devnull, 'w')

    def _run(prog, env, resources):
        master = os.environ['MESOS_MASTER']
        if ':' not in master:
            master += ':5050'

        name = str(uuid.uuid4())
        cwd = os.getcwd()
        prog = "cd %s && %s" % (cwd, prog)

        resources = ';'.join('%s:%s' % (k, v) for k, v in resources.items())
        prog = prog.replace('\'', '\\\'')
        env = json.dumps(env).replace('\'', '\\\'')
        resources = resources.replace('\'', '\\\'')
        cmd = (
            'mesos-execute --master=%s --name=\'%s\''
            ' --command=\'%s\' --env=\'%s\' --resources=\'%s\'' %
            (master, name, prog, env, resources)
        )

        subprocess.check_call(
            cmd,
            shell=True,
            stdout=DEVNULL,
            stderr=subprocess.STDOUT)

    _USE_PYMESOS = False

def get_env():
    # get system envs
    keys = set(['OMP_NUM_THREADS', 'KMP_AFFINITY', 'LD_LIBRARY_PATH'])
    return {k: v for k, v in os.environ.items() if k in keys}


def submit(args):
    def mesos_submit(nworker, nserver, pass_envs):
        """
        customized submit script
        """
        # launch jobs
        for i in range(nworker + nserver):
            resources = {}
            pass_envs['DMLC_ROLE'] = 'server' if i < nserver else 'worker'
            if i < nserver:
                pass_envs['DMLC_SERVER_ID'] = i
                resources['cpus'] = args.server_cores
                resources['mem'] = args.server_memory_mb
            else:
                pass_envs['DMLC_WORKER_ID'] = i - nserver
                resources['cpus'] = args.worker_cores
                resources['mem'] = args.worker_memory_mb

            env = {str(k): str(v) for k, v in pass_envs.items()}
            env.update(get_env())
            prog = ' '.join(args.command)
            thread = Thread(target=_run, args=(prog, env, resources))
            thread.setDaemon(True)
            thread.start()

        return mesos_submit

    if not _USE_PYMESOS:
        logging.warning('No PyMesos found, use mesos-execute instead,'
                        ' no task output available')

    if args.mesos_master:
        os.environ['MESOS_MASTER'] = args.mesos_master

    assert 'MESOS_MASTER' in os.environ, 'No mesos master configured!'

    tracker.submit(args.num_workers, args.num_servers,
                   fun_submit=mesos_submit,
                   pscmd=(' '.join(args.command)))
