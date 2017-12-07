"""
DMLC submission script, SLURM version
"""
# pylint: disable=invalid-name
from __future__ import absolute_import

import subprocess, logging
from threading import Thread
from . import tracker

def get_mpi_env(envs):
    """get the slurm command for setting the environment
    """
    cmd = ''
    for k, v in envs.items():
        cmd += '%s=%s ' % (k, str(v))
    return cmd


def submit(args):
    """Submission script with SLURM."""
    def mpi_submit(nworker, nserver, pass_envs):
        """Internal closure for job submission."""
        def run(prog):
            """run the program"""
            subprocess.check_call(prog, shell=True)

        cmd = ' '.join(args.command)

        pass_envs['DMLC_JOB_CLUSTER'] = 'slurm'

        if args.slurm_worker_nodes is None:
	  nworker_nodes = nworker
        else:
          nworker_nodes=args.slurm_worker_nodes
 

        # start workers
        if nworker > 0:
          logging.info('Start %d workers by srun' % nworker)
          pass_envs['DMLC_ROLE'] = 'worker'
          prog = '%s srun --share --exclusive=user -N %d -n %d %s' % (get_mpi_env(pass_envs), nworker_nodes, nworker, cmd)
          thread = Thread(target=run, args=(prog,))
          thread.setDaemon(True)
          thread.start()


        if args.slurm_server_nodes is None:
	  nserver_nodes = nserver
        else:
          nserver_nodes=args.slurm_server_nodes

        # start servers
        if nserver > 0:
          logging.info('Start %d servers by srun' % nserver)
          pass_envs['DMLC_ROLE'] = 'server'
          prog = '%s srun --share --exclusive=user -N %d -n %d %s' % (get_mpi_env(pass_envs), nserver_nodes, nserver, cmd)
          thread = Thread(target=run, args=(prog,))
          thread.setDaemon(True)
          thread.start()


    tracker.submit(args.num_workers, args.num_servers,
                   fun_submit=mpi_submit,
                   pscmd=(' '.join(args.command)))
