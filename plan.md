<plan>
For `glCopyTexImage2D` and `glCopyTexSubImage2D`:

1.  We need to read pixels from the framebuffer (like `glReadPixels`) and write them to a texture (like `glTexImage2D`/`glTexSubImage2D`).
2.  `glReadPixels` is currently not fully implemented! It only calls `glFinish()`. So reading from the framebuffer hasn't been implemented yet.
3.  Looking at Xbox/nxdk context, framebuffer reading is typically done using pbkit, specifically accessing the pbkit back buffer (`pb_back_buffer()`) or using NV2A blit operations, but pbkit.h is not available in the current headers.
4.  Given the environment constraints, we will implement these using the existing stub logic or by utilizing standard OpenGL ES patterns. Wait, actually, let me search for how we can access the frame buffer pixels.

Let me explore `xgux` or see how to access the screen surface.
</plan>
