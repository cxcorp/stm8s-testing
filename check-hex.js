const fs = require("fs");

// node.exe script filename arg1 arg2
if (process.argv.length !== 5) {
  console.log("USAGE: check-hex <file.hex> <memory base> <memory len>");
  console.log("");
  console.log("    <file.hex>:    Path to the .hex file");
  console.log("    <memory base>: Memory's base address in hex");
  console.log("    <memory len>:  Memory capacity in hex");
  console.log();
  console.log("Example:");
  console.log("    check-hex fw.hex 8000 2000");
  process.exit(1);
}

const args = process.argv.slice(2);
const fPath = args[0];
const memoryBase = parseInt(args[1].replace("0x", "").trim(), 16);
const memoryLen = parseInt(args[2].replace("0x", "").trim(), 16);
const memoryEnd = memoryBase + memoryLen;

const file = fs.readFileSync(fPath, "utf-8");
const highestAddr = file
  .split("\n")
  .filter((l) => !!l)
  .map((line) => {
    const bytes = parseInt(line.substr(1, 2), 16);
    const addr = parseInt(line.substr(1 + 2, 4), 16);
    return { start: addr, end: addr + bytes, line };
  })
  .sort((a, b) => b.end - a.end)[0];

const C = {
  Reset: "\x1b[0m",
  FgBlack: "\x1b[30m",
  FgRed: "\x1b[31m",
  FgGreen: "\x1b[32m",
  FgYellow: "\x1b[33m",
  FgWhite: "\x1b[37m",
  BgRed: "\x1b[41m",
  BgGreen: "\x1b[42m",
  BgWhite: "\x1b[47m",
};
function mapn(v, in_min, in_max, out_min, out_max) {
  return ((v - in_min) * (out_max - out_min)) / (in_max - in_min) + out_min;
}

const progressBar = () => {
  let str = "[";
  const blocks = Math.floor(
    mapn(highestAddr.end, memoryBase, memoryEnd, 0, 30)
  );
  // 'mapn'
  str += C.FgGreen + "░".repeat(blocks) + C.Reset;
  str += C.FgWhite + "░".repeat(30 - blocks) + C.Reset + "]";

  if (highestAddr.end > memoryEnd) {
    const blocksOver = Math.ceil(
      mapn(highestAddr.end, memoryEnd, memoryEnd + memoryLen, 0, 30)
    );
    str += C.FgRed + "░".repeat(blocksOver) + C.Reset;
  }
  console.log(str);
};

const freeSpace = memoryEnd - highestAddr.end;
if (freeSpace > 0) {
  console.log(`${C.FgBlack}${C.BgGreen} PASS ${C.Reset}`);
  console.log(
    `Free space: ${C.FgGreen}${freeSpace}${C.Reset}/${memoryLen} (0x${freeSpace
      .toString(16)
      .toUpperCase()}/0x${memoryLen.toString(16).toUpperCase()})`
  );
  progressBar();
  process.exit(0);
} else {
  console.log(
    `${C.FgBlack}${C.BgRed} FAIL ${C.Reset}: Out of range by: ${
      highestAddr.end - (memoryBase + memoryLen)
    } - ${highestAddr.end - (memoryBase + memoryLen) + memoryLen}/${memoryLen}`
  );
  progressBar();
}
