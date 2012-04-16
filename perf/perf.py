import subprocess

def hosts():
  try:
    slurm_nodes = subprocess.check_output(["scontrol", "show", "hostnames"], stderr=subprocess.PIPE).split()
    return [slurm_nodes[0], slurm_nodes[1]]
  except:
    return ["localhost", "localhost"]

def arguments(a_arguments, b_arguments):
  temp_hosts = hosts()
  if temp_hosts[0] != "localhost" and temp_hosts[0] != "127.0.0.1":
    a_arguments = ["ssh", "-x", temp_hosts[0]] + a_arguments
  if temp_hosts[1] != "localhost" and temp_hosts[1] != "127.0.0.1":
    b_arguments = ["ssh", "-x", temp_hosts[1]] + b_arguments
  return a_arguments, b_arguments
