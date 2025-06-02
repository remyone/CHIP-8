# Chip-8 Emulator (Incomplete)  
*A C implementation of a Chip-8 interpreter. Currently unfinished but documented for transparency.*  

## **Current Status**  
✅ **Implemented**:  
- Basic CPU fetch-decode-execute cycle  
- 31/35 opcodes
- Graphics rendering using ## in console
- RAM and register emulation  

❌ **Unfinished**:  
- **Keyboard input** (there is no key handling, I mean there are keypad opcodes(except FX0A, FX29 and FX33), but keys don't work)
- **Opcode 8XY6**
- **DXYN (draw sprite)** has artifacts (needs pixel collision fix)  
- **Sound timer** (stubbed but non-functional)  

## **Why This Is Unfinished**  
I paused development due to:  
1. **Complexity of edge cases** (e.g., DXYN wrapping) burning me out.  
2. **Shifted focus** to lower-level projects (e.g., memory editors).  

## **How to Run**  
```bash  
make main
./chip8 /path/to/ROM
```
