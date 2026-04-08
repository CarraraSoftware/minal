print("\x1B[30m")
for idx in range(256):
    print(f"\x1B[48;5;{idx}m█\x1B[49m", end="")

# for red in range(0, 6):
#     for blue in range(0, 6):
#         for green in range(0, 6):
#             r = red   * 40 + 55
#             b = blue  * 40 + 55
#             g = green * 40 + 55
#             idx += 1 
#         print()
#
# # 232 - 255
# for gray in range(0, 24):
#     level = gray * 10 + 8
#     code = 232 + gray
#     print(f"\x1B[38;2;{r};{g};{b}m█\x1B[m")
