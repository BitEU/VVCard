import random
from datetime import datetime
from faker import Faker

# Initialize the Faker generator
fake = Faker()

NUM_CONTACTS = 200  # Change this to generate more or fewer contacts

def random_uid():
    """Helper to generate a random UUID-like string."""
    return f"urn:uuid:{random.getrandbits(32):08x}-{random.getrandbits(16):04x}-{random.getrandbits(16):04x}-{random.getrandbits(16):04x}-{random.getrandbits(48):012x}"

def now_utc():
    """Returns the current UTC time in the required VCF format."""
    return datetime.utcnow().strftime("%Y%m%dT%H%M%SZ")

def escape(val):
    """Escapes special characters in VCF fields."""
    return val.replace('\\', '\\\\').replace(',', '\\,').replace(';', '\\;').replace('\n', '\\n')

# The VCF template remains the same
TEMPLATE = '''BEGIN:VCARD
VERSION:4.0
UID:{uid}
REV:{rev}
PRODID:-//VCard Contact Manager//EN
FN:{fn}
N:{family};{given};{additional};{prefix};{suffix}
NICKNAME:{nickname}
EMAIL;TYPE=HOME:{email_home}
EMAIL;TYPE=WORK:{email_work}
EMAIL;TYPE=OTHER:{email_other}
TEL;TYPE=HOME:{phone_home}
TEL;TYPE=WORK:{phone_work}
TEL;TYPE=CELL:{phone_cell}
ADR;TYPE=HOME:;;{street};{city};{region};{postal};{country}
ORG:{org}
TITLE:{title}
ROLE:{role}
BDAY:{bday}
ANNIVERSARY:{anniv}
GENDER:{gender}
LANG:{lang}
TZ:{tz}
URL:{url}
URL;TYPE=linkedin:{linkedin}
URL;TYPE=twitter:{twitter}
IMPP:{impp}
GEO:{geo}
BIRTHPLACE:{birthplace}
X-BIRTHPLACE-GEO:{birthplace_geo}
DEATHPLACE:{deathplace}
X-DEATHPLACE-GEO:{deathplace_geo}
DEATHDATE:{deathdate}
RELATED:{related}
EXPERTISE:{expertise}
HOBBY:{hobby}
INTEREST:{interest}
CATEGORIES:{categories}
NOTE:{note}
KIND:{kind}
END:VCARD
'''

def generate_random_contact():
    """Generates a dictionary of random contact information using Faker."""
    given_name = fake.first_name()
    family_name = fake.last_name()
    username = f"{given_name.lower()}{family_name.lower()}{random.randint(1,99)}"
    
    lat, lng = fake.latitude(), fake.longitude()
    
    contact_data = {
        "fn": f"{given_name} {family_name}",
        "family": family_name,
        "given": given_name,
        "additional": fake.first_name() if random.choice([True, False]) else "",
        "prefix": fake.prefix(),
        "suffix": fake.suffix() if random.choice([True, False]) else "",
        "nickname": username,
        "email_home": fake.email(),
        "email_work": fake.company_email(),
        "email_other": fake.email(),
        "phone_home": fake.phone_number(),
        "phone_work": fake.phone_number(),
        "phone_cell": fake.phone_number(),
        "street": fake.street_address(),
        "city": fake.city(),
        "region": fake.state_abbr(),
        "postal": fake.zipcode(),
        "country": "USA",
        "org": fake.company(),
        "title": fake.job(),
        "role": fake.job(),
        "bday": fake.date_of_birth(minimum_age=18, maximum_age=90).strftime('%Y%m%d'),
        "anniv": fake.date_this_decade().strftime('%Y%m%d'),
        "gender": random.choice(['M', 'F', 'O', '']),
        "lang": fake.language_code(),
        "tz": fake.timezone(),
        "url": fake.url(),
        "linkedin": f"https://linkedin.com/in/{username}",
        "twitter": f"https://twitter.com/{username}",
        "impp": f"xmpp:{username}@chat.com",
        "geo": f"geo:{lat},{lng}",
        "birthplace": fake.city(),
        "birthplace_geo": f"geo:{fake.latitude()},{fake.longitude()}",
        "deathplace": "",  # Typically empty
        "deathplace_geo": "", # Typically empty
        "deathdate": "",    # Typically empty
        "related": fake.name(),
        "expertise": f"{fake.bs()}, {fake.bs()}",
        "hobby": fake.bs().capitalize(),
        "interest": f"{fake.bs()}, {fake.bs()}",
        "categories": ",".join(random.sample(["Friends", "Work", "Family", "Networking"], k=2)),
        "note": fake.sentence(),
        "kind": "individual"
    }
    return contact_data

def make_contact_vcard(data):
    """Formats a contact data dictionary into a VCF string."""
    data["uid"] = random_uid()
    data["rev"] = now_utc()
    
    # Escape all string values in the dictionary
    for k, v in data.items():
        if isinstance(v, str):
            data[k] = escape(v)
            
    return TEMPLATE.format(**data)

# Main execution block
if __name__ == "__main__":
    with open("contacts.vcf", "w", encoding="utf-8") as f:
        print(f"Generating {NUM_CONTACTS} contacts...")
        for i in range(NUM_CONTACTS):
            random_data = generate_random_contact()
            vcard = make_contact_vcard(random_data)
            f.write(vcard)
    print("Successfully created contacts.vcf")