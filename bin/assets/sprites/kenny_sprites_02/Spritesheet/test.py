import xml.etree.ElementTree as ET

# Parse the XML file
tree = ET.parse('sheet.xml')
root = tree.getroot()

# Create the Lua table
table = {}

# Get the image path
table['imagePath'] = root.attrib['imagePath']

# Iterate through the SubTexture elements
for sub_texture in root.iter('SubTexture'):
    name = sub_texture.attrib['name']
    x = int(sub_texture.attrib['x'])
    y = int(sub_texture.attrib['y'])
    width = int(sub_texture.attrib['width'])
    height = int(sub_texture.attrib['height'])

    # Add the SubTexture to the table
    table[name] = {
        'x': x,
        'y': y,
        'width': width,
        'height': height
    }

# Write the Lua table to a file
with open('result.lua', 'w') as f:
    f.write(str(table))
