var plask = require('plask');

plask.simpleWindow({
  settings: {
    width: 1920,
    height: 1080,
    type: '3d',  // Create an OpenGL window.
    vsync: true,  // Prevent tearing.
    // multisample: true,  // Anti-alias.
    // fullscreen: true
  },

  init: function() {
    var gl = this.gl;

    this.framerate(60);

    gl.viewport(0, 0, this.width, this.height);
    gl.clearColor(230/255, 230/255, 230/255, 1.0);

    this.nvg = new PlaskRawMac.NVG();

    this.time = this.lastTime = Date.now();
    this.frames = this.fps = 0;
  },

  printFPS: function() {
    this.time = Date.now();
    this.frames++;

    if(this.time > this.lastTime + 1000){
      this.fps = Math.round((this.frames * 1000) / (this.time - this.lastTime));
      this.lastTime = this.time;
      this.frames = 0;
      console.log(this.fps);
    }
  },

  draw: function() {
    var gl = this.gl, nvg = this.nvg;

    // Clear the background to gray.  The depth buffer isn't really needed for
    // a single triangle, but generally the depth buffer should also be cleared.
    gl.clear(gl.COLOR_BUFFER_BIT | gl.STENCIL_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    nvg.resetTransform();
    nvg.beginFrame(this.width, this.height, 1, PlaskRawMac.NVG_STRAIGHT_ALPHA);

    nvg.translate(this.width/2, this.height/2);
    nvg.rotate(Math.sin(this.framenum/100));

    nvg.beginPath();
    nvg.ellipse(0, 0, 200, 200);
    nvg.fillColor(1.0, 0, 1.0, 1.0);
    nvg.fill();

    nvg.beginPath();
    nvg.moveTo(0, 0);
    nvg.lineTo(600, 700);
    nvg.strokeWidth(20.0);
    nvg.strokeColor(1.0, 0, 0, 1.0);
    nvg.stroke();

    nvg.resetTransform();
    nvg.translate(Math.sin(this.frametime) * this.width/2 + this.width/2, 0);
    nvg.beginPath();
    nvg.moveTo(0, 0);
    nvg.lineTo(0, this.height);
    nvg.strokeColor(0, 0, 0, 1.0);
    nvg.stroke();

    nvg.resetTransform();
    nvg.translate(this.width/2, this.height/2);
    nvg.translate(Math.cos(this.frametime) * 400, Math.sin(this.frametime) * 400);
    nvg.beginPath();
    nvg.ellipse(0, 0, 50, 50);
    nvg.fillColor(1.0, 1.00, 1.0, 1.0);
    nvg.fill();


    nvg.endFrame();

    this.printFPS();
  }
});
