## Description
Implement a kernel module `kfetch_mod`.
`kfetch_mod` is a character device driver that creates a device called `/dev/kfetch`.
The user-space program `kfetch` can retrieve the system information by reading from this device.

### Result
<img width="437" alt="image" src="https://github.com/TainanKyle/system_information_fetching/assets/150419874/9152b076-080a-4646-8646-ab7019100a0a">

<img width="436" alt="image" src="https://github.com/TainanKyle/system_information_fetching/assets/150419874/e0507079-04ce-47d1-9261-1f9bbba6e0b7">
