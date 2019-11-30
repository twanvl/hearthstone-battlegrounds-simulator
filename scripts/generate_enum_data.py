# This script generates the enums.hpp and enum_data.cpp files
# As input, uses the HearthDb database

import xml.etree.ElementTree as ET

# ------------------------------------------------------------------------------
# Extracting info from CardDefs.xml
# ------------------------------------------------------------------------------

def get_tag(e,n):
  return e.find("Tag[@name='{}']".format(n))

def get_tag_value(e,n,default=None):
  tag = get_tag(e,n)
  if tag is None:
    return default
  else:
    return tag.attrib['value']

def get_tag_int(e,n,default=0):
  return int(get_tag_value(e,n,default))

def get_tag_loc_string(e,n,lang='enUS'):
  tag = e.find("Tag[@name='{}']/{}".format(n,lang))
  if tag is None:
    return None
  else:
    return tag.text

def enum_name(n):
  out = ""
  word_start = True
  for c in n:
    if c in "- ":
      word_start = True
    elif c.isalnum():
      if word_start:
        out += c.upper()
      else:
        out += c
      word_start = False
  return out

class Entity:
  def __init__(self,e):
    self.e = e
  def get_tag(self,x):
    return get_tag(self.e,x)
  def get_int(self,x):
    return get_tag_int(self.e,x)
  def get_bool(self,x):
    return get_tag_int(self.e,x) == 1
  @property
  def name(self):
    return get_tag_loc_string(self.e,'CARDNAME')
  @property
  def id(self):
    return self.e.attrib["CardID"]
  @property
  def enum(self):
    return enum_name(self.name)
  @property
  def tier(self):
    return get_tag_int(self.e,"TECH_LEVEL",1)
  @property
  def custom(self):
    return False
  @property
  def cleave(self):
    text = get_tag_loc_string(self.e,"CARDTEXT")
    return text is not None and "minions next to whomever" in text
  @property
  def token(self):
    return not self.get_bool("IS_BACON_POOL_MINION")
  @property
  def tribe(self):
    tribe = get_tag_int(self.e,"CARDRACE")
    if tribe in tribes:
      return tribes[tribe]
    else:
      return str(tribe)

extra_names = ["Safeguard","Plant"]

def extract_data():
  # Load xml file
  cards = ET.parse('hsdata/CardDefs.xml')
  defs = cards.getroot()

  # Collect minions and heroes
  minions = dict()
  heroes = []

  # Loop over entities, collect minion info
  for e in defs.iterfind("Entity"):
    e = Entity(e)
    name = e.name
    if name is None: continue
    if e.get_int("CARDTYPE") == 4: # minion
      if not (e.get_int("TECH_LEVEL") or name in extra_names): continue
      if e.id == "TRLA_149": continue # there are two versions of Ghastcoiler
      golden = "BaconUps" in e.id
      if not e.name in minions:
        minions[e.name] = [None,None]
      if minions[e.name][1 if golden else 0] is not None:
        print("Warning: duplicate minion:",name, minions[e.name][1 if golden else 0].id, e.id)
      minions[e.name][1 if golden else 0] = e
    elif "TB_BaconShop_HERO" in e.id and e.id != "TB_BaconShop_HERO_PH" and e.get_int("CARDTYPE") == 3: # hero
      heroes.append(e)
      # find heropower
      hp = e.get_tag("HERO_POWER")
      if hp is None:
        e.hp = NoneEntity()
        print("No hero power for",name)
      else:
        e.hp = Entity(defs.find("Entity[@CardID='{}']".format(hp.attrib["cardID"])))

  return minions,heroes

# ------------------------------------------------------------------------------
# Extra data
# ------------------------------------------------------------------------------

tribes = {0:"None", 14:"Murloc", 15:"Demon", 17:"Mech", 20:"Beast", 24:"Dragon", 26:"All"}

class CustomEntity:
  id = None
  tribe = "None"
  tier = 1
  attack = 0
  health = 0
  cleave = False
  token = False
  custom = True
  def __init__(self, name):
    self.name = name
  def get_tag(self,x):
    return get_tag(self.e,x)
  def get_int(self,x):
    if x == "ATK": return self.attack
    if x == "HEALTH": return self.health
    return 0
  def get_bool(self,x):
    return False
  @property
  def enum(self):
    return enum_name(self.name)
  @property
  def hp(self):
    return NoneEntity()

class NoneEntity (CustomEntity):
  def __init__(self):
    self.name = "(none)"
    self.tier = 0
    self.custom = False

def add_custom_minions(minions):
  minions.insert(0,[NoneEntity(),None])
  mama = CustomEntity("Pre-nerf Mama Bear")
  mama.tier = 6
  mama.attack = 5
  mama.health = 5
  mama.tribe = "Beast"
  minions.append([mama,None])

def add_custom_heroes(heroes):
  heroes.insert(0,NoneEntity())

# ------------------------------------------------------------------------------
# enums.hpp
# ------------------------------------------------------------------------------

# Sort minions by tier and name
def get_not_none(xs):
  if xs[0] is None:
    return xs[1]
  else:
    return xs[0]

def sort_minions(minions):
  return sorted(minions.values(), key = lambda x: (x[0].token, x[0].tier, x[0].name))

def sort_heroes(heroes):
  return sorted(heroes, key = lambda x: x.name)

def write_enums_hpp(minions, heroes):
  with open("src/enums.hpp","wt", encoding="utf-8") as f:
    f.write("#pragma once\n\n")
    f.write("// -----------------------------------------------------------------------------\n")
    f.write("// THIS FILE IS AUTOGENERATED\n")
    f.write("// see generate_enum_data.py\n")
    f.write("// -----------------------------------------------------------------------------\n\n")

    f.write("// -----------------------------------------------------------------------------\n")
    f.write("// Tribes\n")
    f.write("// -----------------------------------------------------------------------------\n\n")
    f.write("enum class Tribe {\n")
    for i in sorted(tribes):
      f.write("  {},\n".format(tribes[i]))
    f.write("};\n\n")
    f.write("const int Tribe_count = {};\n\n".format(len(tribes)))

    f.write("// -----------------------------------------------------------------------------\n")
    f.write("// Minion types\n")
    f.write("// -----------------------------------------------------------------------------\n\n")
    f.write("enum class MinionType : unsigned char {\n")
    tier = 0
    for m in minions:
      e = m[0]
      if e.tier != tier:
        if e.token:
          f.write("  // Tokens\n")
        elif e.custom:
          f.write("  // Custom\n")
        else:
          f.write("  // Tier {}\n".format(e.tier))
        tier = e.tier
      f.write("  {},\n".format(e.enum))
    f.write("};\n\n")
    f.write("const int MinionType_count = {};\n\n".format(len(minions)))

    f.write("// -----------------------------------------------------------------------------\n")
    f.write("// Heroes and hero powers\n")
    f.write("// -----------------------------------------------------------------------------\n\n")
    f.write("enum class HeroType {\n")
    for e in heroes:
      f.write("  {},\n".format(e.enum))
    f.write("};\n\n")
    f.write("const int HeroType_count = {};\n".format(len(heroes)))

# ------------------------------------------------------------------------------
# enum_data.cpp
# ------------------------------------------------------------------------------

def cbool(x):
  if x:
    return "true"
  else:
    return "false"

def cstr(x):
  if x is None:
    return "nullptr"
  else:
    return "\"" + x + "\""

def write_enum_data_cpp(minions, heroes):
  with open("src/enum_data.cpp","wt", encoding="utf-8") as f:
    f.write("#include \"enums.hpp\"\n")
    f.write("#include \"minion_info.hpp\"\n\n")
    f.write("// -----------------------------------------------------------------------------\n")
    f.write("// THIS FILE IS AUTOGENERATED\n")
    f.write("// see generate_enum_data.py\n")
    f.write("// -----------------------------------------------------------------------------\n\n")
    f.write("\n")
    f.write("// -----------------------------------------------------------------------------\n")
    f.write("// Tribe information\n")
    f.write("// -----------------------------------------------------------------------------\n\n")
    f.write("const char* tribe_names[] = {\n")
    for i in sorted(tribes):
      f.write("  {},\n".format(cstr(tribes[i])))
    f.write("};\n\n")

    f.write("// -----------------------------------------------------------------------------\n")
    f.write("// Minion information\n")
    f.write("// -----------------------------------------------------------------------------\n\n")
    f.write("const MinionInfo minion_info[] = {\n")
    for m in minions:
      e = m[0]
      f.write("  {{{}, {{{},{}}}, {}, Tribe::{}, {},{}, {},{},{},{},{}}},\n".format(
        cstr(e.name), cstr(e.id), cstr(m[1].id if m[1] is not None else None),
        e.tier, e.tribe,
        e.get_int("ATK"), e.get_int("HEALTH"),
        cbool(e.get_bool("TAUNT")), cbool(e.get_bool("DIVINE_SHIELD")), cbool(e.get_bool("POISONOUS")), cbool(e.get_bool("WINDFURY")),
        cbool(e.cleave)
      ))
    f.write("};\n\n")

    f.write("// -----------------------------------------------------------------------------\n")
    f.write("// Lists of minions\n")
    f.write("// -----------------------------------------------------------------------------\n\n")
    write_minion_list(f, minions, "one_cost", lambda e: e.get_int("COST") == 1 and not e.token)
    write_minion_list(f, minions, "two_cost", lambda e: e.get_int("COST") == 2 and not e.token)
    write_minion_list(f, minions, "four_cost", lambda e: e.get_int("COST") == 4 and not e.token)
    write_minion_list(f, minions, "deathrattle", lambda e: e.get_int("DEATHRATTLE") and not e.token)
    write_minion_list(f, minions, "legendary", lambda e: e.get_int("RARITY") == 5 and not e.token)

    f.write("// -----------------------------------------------------------------------------\n")
    f.write("// Hero / hero power information\n")
    f.write("// -----------------------------------------------------------------------------\n\n")
    f.write("const HeroInfo hero_info[] = {\n")
    for e in heroes:
      hp = e.hp
      f.write("  {{{},{},{{{},{}}}}},\n".format(cstr(e.name),cstr(e.id), cstr(hp.name),cstr(hp.id)))
    f.write("};\n")

def write_minion_list(f, minions, name, filter):
  f.write("static const MinionType {}_minions[] = {{\n".format(name))
  for m in minions:
    e = m[0]
    if filter(e):
      f.write("  MinionType::{},\n".format(e.enum))
  f.write("}};\n".format(name))
  f.write("MinionType random_{}_minion(RNG& rng) {{\n".format(name))
  f.write("  return random_element({}_minions, rng);\n".format(name))
  f.write("}\n\n")

# ------------------------------------------------------------------------------
# Main
# ------------------------------------------------------------------------------

minions, heroes = extract_data()
print("Found",len(minions),"minions", len(heroes),"heroes")

minions = sort_minions(minions)
add_custom_minions(minions)
heroes = sort_heroes(heroes)
add_custom_heroes(heroes)

write_enums_hpp(minions, heroes)
write_enum_data_cpp(minions, heroes)
