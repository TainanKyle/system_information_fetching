## Description
Implement a kernel module `kfetch_mod`.
`kfetch_mod` is a character device driver that creates a device called `/dev/kfetch`.
The user-space program `kfetch` can retrieve the system information by reading from this device.
