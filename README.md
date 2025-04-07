# EspressiScale

EspressiScale is a minimalist and affordable espresso scale designed to provide accurate measurements along with a built-in timer – perfect for both home and small coffee shop use.

## Features

- **Accurate Weighing:** Provides precise measurements for your espresso brewing.
- **Built-in Timer:** Monitors brewing time to ensure optimal extraction.
- **Minimalist Design:** A simple and modern solution focusing on essential features.

## Makerworld
This was also published on [makerworld](https://makerworld.com/en/models/1212476-espressiscale-small-minimalist-espresso-scale#profileId-1227630) for easy 3D printing.

## Bill of Materials (BOM)

| Quantity | Component                   | Details/Notes                                      |
|----------|-----------------------------|----------------------------------------------------|
| 1        | [Lilygo T-Touch bar](https://lilygo.cc/products/t-touch-bar?variant=42880705102005)          |                                                    |
| 1        | Battery                     | Any 3.7v rechargeable lithium battery (<10x34x48)       |
| 1        | [500g Load Cell](https://www.aliexpress.com/item/1005006390450639.html?spm=a2g0o.productlist.main.1.140c3ad4i723T1&algo_pvid=c0719cfc-035d-4043-bfff-a19c34d26db9&algo_exp_id=c0719cfc-035d-4043-bfff-a19c34d26db9-0&pdp_ext_f=%7B%22order%22%3A%2214%22%2C%22eval%22%3A%221%22%7D&pdp_npi=4%40dis%21EUR%213.13%211.24%21%21%2124.03%219.53%21%40211b61bb17420425268903777eb948%2112000036996330117%21sea%21NO%210%21ABX&curPageLogUid=1wjYKMejCK7S&utparam-url=scene%3Asearch%7Cquery_from%3A)            |                                                    |
| 1        | [HX711](https://www.aliexpress.com/item/1005006293368575.html?spm=a2g0o.productlist.main.19.4b5474a6I3ualW&algo_pvid=8b13f76d-943b-4f75-8bf8-1741b29a8fbf&algo_exp_id=8b13f76d-943b-4f75-8bf8-1741b29a8fbf-9&pdp_ext_f=%7B%22order%22%3A%22487%22%2C%22eval%22%3A%221%22%7D&pdp_npi=4%40dis%21EUR%212.05%210.93%21%21%2115.75%217.18%21%40211b430817420419468811798eb8b9%2112000036639761167%21sea%21NO%210%21ABX&curPageLogUid=WayeqeRbhY4T&utparam-url=scene%3Asearch%7Cquery_from%3A)                     | High precision (available on AliExpress)           |
|1         | [USB type C port](https://a.aliexpress.com/_EyspPbK)             | Not yet implimented
| 4        | [M3x3.0 Threaded Inserts](https://cnckitchen.store/products/heat-set-insert-m3-x-3-short-version-100-pieces)     | (I used CNCkitchen)                                |
| 2        | M3x10 Screws                |                                                    |
| 2        | M3x12 Screws                |                                                    |
| 2        | M3x12 Countersunk Screws    |                                                    |
| -        | Heat Shrink                 |                                                    |

### Optional Components

| Quantity | Component                   | Details/Notes                                      |
|----------|-----------------------------|----------------------------------------------------|
| 2        | Extra Heated Inserts        |                                                    |
| 2        | M3x6 Screws                 |                                                    |



## Installation

1. **PlatformIO setup in Visual Studio:**

   Make sure you have PlatformIO installed in Visual Studio. You'll also need espressif platform installed.
   
3. **Clone the repository:**

   ```bash
   git clone https://github.com/whauu/EspressiScale.git

4. **Navigate to the project folder**

   ```bash
   cd EspressiScale

5. **Open project in PlatformIO**
   
6. **Erase flash**
   
7. **Build filesystem**
   
8. **Upload filesystem**
   
9. **Build**

10. **Upload**



## Usage
**Touch controls:**
- **Left Display:** Starts and stops timer
- **Right Display:** Tares weight and resets timer
  
**Power:**
  - Touch the display anywhere to wake it up
  - The scale will automatically enter deep sleep after 5min with no use

**Update:**
   - Update using "scaleIP"/update
   - Build updated project
   - Upload the firmware.bin file
     <p align="center">
<img src="https://github.com/user-attachments/assets/cd5b869b-26ec-4d18-aa3c-554c78ec7306" alt="Screenshot" />
</p>

[PrettyOTA](https://github.com/LostInCompilation/PrettyOTA.git)

## License
This project is licensed under my own license

## [Discord](https://discord.gg/vpcPMjV8)

## Contact
If you have any questions or suggestions, you can:
- Open an issue in the GitHub repository
- Contact me directly at [whauu@espressiscale.com]
