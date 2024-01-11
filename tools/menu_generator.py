#!/usr/bin/env python3

import xml.etree.cElementTree as ET
import xml.etree.ElementTree as etree

def stringtoken(value: str) -> str:
    result = '"'
    for byte in value.encode():
        # Convert non-ASCII, '"', and '\' to hex escape.
        if 0x20 <= byte <= 0x7e and byte != ord('"') and byte != ord('\\'):
            result += chr(byte)
        else:
            result += '\\x{:02x}'.format(byte)
    result += '"'
    return result

def parsechoicewidget(name: str, widget: etree.Element) -> str:
    # Create list of choices. No need to forward declare.
    result = 'static const char *const {}_choices[] = {{\n'.format(name)
    numchoices = 0
    for choice in widget.findall('choice'):
        numchoices += 1
        result += '    '
        result += stringtoken(choice.text)
        result += ',\n'
    # Assemble widget.
    result += '};\n\n'
    result += 'static const menuwidget_t {} = {{\n'.format(name)
    result += '    .type = WIDGET_CHOICE,\n'
    result += '    .choice = {\n'
    result += '        .ref = &{},\n'.format(widget.attrib['ref'])
    result += '        .numchoices = {},\n'.format(numchoices)
    result += '        .choices = {}_choices,\n'.format(name)
    result += '     },\n'
    result += '};\n\n'
    return result

def parsewidget(name: str, widget: etree.Element) -> str:
    match widget.attrib['type']:
        case 'choice':
            return parsechoicewidget(name, widget)
        case _:
            raise RuntimeError()

def parsescreen(screen: etree.Element) -> str:
    name = screen.attrib['id']
    # Build the items first.
    result = 'static const menuitem_t items_{}[] = {{\n'.format(name)
    widgets = ''
    numitems = 0
    for item in screen.findall('item'):
        numitems += 1
        # Have widget?
        widget_name = None
        if widget := item.find('widget'):
            widget_name = 'widget_{}_{}'.format(name, len(widgets))
            widgets += parsewidget(widget_name, widget)
        # Build item.
        result += '    {\n'
        result += '        .label = {},\n'.format(stringtoken(item.findtext('label')))
        result += '        .target = {},\n'.format('&menu_' + item.attrib['target'] if 'target' in item.attrib else 'NULL')
        result += '        .widget = {},\n'.format('&' + widget_name if widget_name is not None else 'NULL')
        result += '    },\n'
    result += '};\n\n'
    # Next define the screen.
    result += 'static const menu_t menu_{} = {{\n'.format(name)
    result += '    .parent = {},\n'.format('&menu_' + screen.attrib['parent'] if 'parent' in screen else 'NULL')
    result += '    .numitems = {},\n'.format(numitems)
    result += '    .items = items_{},\n'.format(name)
    result += '};\n\n'
    # Prepend the widgets.
    return widgets + result

# Parse the XML for the menu defines.
def parsemenu(filename: str):
    tree = ET.parse(filename)
    root = tree.getroot()
    # Forward declarations.
    forwarddecls = ''
    # Build list of screen definitions.
    result = ''
    for screen in root.findall('screen'):
        # Create a forward declaration of this menu screen.
        forwarddecls += 'static const menu_t menu_{};\n'.format(screen.attrib['id'])
        result += parsescreen(screen)
    # Preprend forward declarations.
    return forwarddecls + result

result = parsemenu('menu/menu.xml')
with open('src/g_menugen.inl', 'w') as file:
    file.write(result)
