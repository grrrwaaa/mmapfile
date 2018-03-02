# mmapfile

Node.js addon to map files as shared memory buffers, for unix + windows.

Using mmap is fast and allows shared memory between processes.

Usage:

```
const mf = require('bindings')('mmapfile');

// open a file for read/write & map to a Buffer
let wb = mf.openSync("buf.txt", 8, "r+");		
// write to it:
wb.fill('-');

// open the file for read only & map to a Buffer
// (could be in an entirely different process)
let rb = mf.openSync("buf.txt", 8);
console.log(rb.byteLength); // 8
console.log(rb.toString('ascii')); // "--------" 
```