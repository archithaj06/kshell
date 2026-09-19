# kshell-vm

Ubuntu Server 24.04 VirtualBox guest used for building and loading the
`kshellnote` kernel module. Kernel work never runs on the host.

| Setting | Value |
|---|---|
| VM name | `kshell-vm` |
| User / password | `kshell` / `kshell` (throwaway; change with `passwd`) |
| SSH | `ssh -i ~/.ssh/kshell_vm -p 2222 kshell@127.0.0.1` (NAT port-forward 2222 -> 22) |
| Resources | 1 vCPU, 2.5 GB RAM, 20 GB disk |

The VM was created with `VBoxManage unattended install` using
`ubuntu_autoinstall_user_data`, which installs `build-essential`,
`linux-headers-generic`, `git`, `make` and the SSH public key.

Note: on this host Hyper-V is active (WSL2/Docker), so VirtualBox runs in
its slower NEM fallback mode; the VM is single-vCPU because SMP guests hang
under NEM.

## Helpers

```bash
./vm/ssh.sh                # shell into the VM
./vm/ssh.sh 'dmesg | tail' # one-off command
./vm/sync.sh               # copy the repo into ~/kshell on the VM
```

## Snapshots

Before loading a new module version:

```bash
VBoxManage snapshot kshell-vm take "before-insmod"
VBoxManage snapshot kshell-vm restore "before-insmod"   # if it hard-locks
```
