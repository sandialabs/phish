import os
import subprocess

def hosts():
  try:
    slurm_nodes = subprocess.check_output(["scontrol", "show", "hostnames"], stderr=subprocess.PIPE).split()
    return [slurm_nodes[0], slurm_nodes[1]]
  except:
    return ["localhost", "localhost"]

def arguments(a_arguments, b_arguments):
  def full_arguments(host, arguments):
    if host == "localhost" or host == "127.0.0.1":
      return arguments
    else:
      return ["ssh", "-x", host, "env", "PATH=%s" % os.environ["PATH"], "PYTHONPATH=%s" % os.environ["PYTHONPATH"], "LD_LIBRARY_PATH=%s" % os.environ["LD_LIBRARY_PATH"]] + arguments

  temp_hosts = hosts()
  return full_arguments(temp_hosts[0], a_arguments), full_arguments(temp_hosts[1], b_arguments)
