import re

# Read the YAML file
with open('m5poe_user.yaml', 'r') as f:
    content = f.read()

# Find all register lines and swap convid with registryID
def swap_convid_registryid(match):
    line = match.group(0)
    
    # Extract the values
    mode_match = re.search(r'mode:\s*(\d+)', line)
    convid_match = re.search(r'convid:\s*(0x[0-9A-Fa-f]+|\d+)', line)
    offset_match = re.search(r'offset:\s*(\d+)', line)
    registryid_match = re.search(r'registryID:\s*(0x[0-9A-Fa-f]+|\d+)', line)
    rest_match = re.search(r'(dataSize:.*)', line)
    
    if mode_match and convid_match and offset_match and registryid_match and rest_match:
        mode = mode_match.group(1)
        convid = convid_match.group(1)
        offset = offset_match.group(1)
        registryid = registryid_match.group(1)
        rest = rest_match.group(1)
        
        # Swap convid and registryID
        new_line = f"    - {{ mode: {mode}, convid: {registryid}, offset: {offset}, registryID: {convid}, {rest}"
        return new_line
    
    return line

# Apply the swap to all register lines
pattern = r'    - \{ mode: \d+, convid: (?:0x[0-9A-Fa-f]+|\d+), offset: \d+, registryID: (?:0x[0-9A-Fa-f]+|\d+),.*\}'
new_content = re.sub(pattern, swap_convid_registryid, content)

# Write back to the file
with open('m5poe_user.yaml', 'w') as f:
    f.write(new_content)

print("✓ Successfully swapped convid and registryID in all registers")
