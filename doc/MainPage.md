Doxygen documentation of kernel module and user app source for
displaying images onto WS2812 diodes panels.

Implemented module uses framebuffer as a medium of drawing
image onto led panel (tested for 8x8 panels). Way of
initializing framebuffer is described in \ref frame_buffer_init.

Framebuffer pixel data is beeing processed by
work function (\ref WS2812_convert_work_fun) in work queue, so processing will not
take a lot of time in kernel space.

\ref module_init
\htmlonly
<details open>
  <summary><strong>Graph representation of what is going on in module</strong></summary>
\endhtmlonly


\htmlonly
<div class="mermaid">
---
title: Example usage
config:
  mirrorActors: false
---
sequenceDiagram
    box rgb(255, 232, 247) Kernel Space
    participant SPI
    participant framebuffer
    participant Module
    end
    box rgb(235, 252, 255) User Space
    participant User App
    participant User
    end
    User-->>Module: modprobe
    Note left of User: or dts init
    critical initialize module
    Module->>Module: initialize
    Note left of Module: module_init(void)
    Note left of Module: driver_setup(spi_device*)
    Module->>framebuffer: create & alloc memory
    Module->>Module: create, setup workqueue & alloc work buffer
    Module->>SPI: create & setup spi
    end
    User->>User App: execute
    User App->>framebuffer: map memory
    Note right of framebuffer: overloaded fb_mmap
    loop draw/display loop
    User App->>framebuffer: draw into mapped memory
    User App->>framebuffer: Call Synchronize call
    Note right of Module: ioctl(WS_IO_PROCESS_AND_SEND)
    framebuffer->>Module: process ioctl call
    activate Module
    Module-->>framebuffer: Copy visible pixel data <br> to prevent race condition
    Module->>Module: Process data
    Note left of Module: Convert pixel data into <br> SPI transfer data <br><br> deffered process
    Module-->>-SPI: start transfer
    end
</div>
\endhtmlonly

\htmlonly
</details>
\endhtmlonly