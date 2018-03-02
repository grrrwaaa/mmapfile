const mf = require('./index.js');
const assert = require('assert');

let bufname = "testbuf.tmp";		
let len = 8;

// open a file for read/write & map to Buffer
let wb = mf.openSync(bufname, len, "r+");		

assert(wb);
assert(Buffer.isBuffer(wb));
assert(wb.byteLength == len);

wb.fill('-');
assert(wb.toString('ascii') == "--------");

// open a file for read only & map to Buffer
let rb = mf.openSync(bufname, len);
assert(rb);
assert(Buffer.isBuffer(rb));
assert(rb.byteLength == len);
assert(rb.toString('ascii') == "--------");

