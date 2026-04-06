/** @file events.h
 *  @brief News event definitions and triggering helpers.
 *
 *  Each `NewsEvent` carries a full date (year, month, day), a short headline,
 *  and a one-sentence body.  `ALL_EVENTS` is sorted chronologically; callers
 *  that scan it with a monotonically advancing index can stop at the first
 *  event whose date has not yet been reached.
 *
 *  No raylib dependency — safe to include in unit tests.
 */

#pragma once

#include "gamestate.h"

// ── Data types ────────────────────────────────────────────────────────────────

/**
 * @brief A single historical news event tied to a specific calendar date.
 */
struct NewsEvent {
    int         year;       ///< Four-digit trigger year
    int         month;      ///< Trigger month [1, 12]
    int         day;        ///< Trigger day   [1, 31]
    const char* headline;   ///< Short display title
    const char* body;       ///< One-sentence flavour description
};

// ── Event catalogue ───────────────────────────────────────────────────────────

/**
 * @brief All known news events, sorted in ascending chronological order.
 *
 * The list must remain sorted — `update_news_events()` in libinput.cpp relies
 * on a monotonically advancing index and stops at the first unmet date.
 * Days marked "(approx)" indicate that the exact day is uncertain; month-level
 * accuracy is guaranteed.
 */
static const NewsEvent ALL_EVENTS[] = {
    {1936, 11,  1, "Turing Publishes 'On Computable Numbers'",
               "Alan Turing lays the theoretical foundation for modern computing and the concept of a universal machine."},
    {1940,  3, 18, "Turing's Bombe Cracks Enigma at Bletchley Park",
               "Alan Turing's electromechanical Bombe decrypts Nazi Enigma communications, giving the Allies a decisive intelligence advantage in WWII."},
    {1946,  2, 15, "ENIAC Becomes Operational",
               "The first general-purpose electronic computer is dedicated at the University of Pennsylvania."},
    {1947, 12, 16, "Transistor Invented at Bell Labs",
               "Shockley, Bardeen, and Brattain demonstrate the point-contact transistor, enabling miniaturized electronics."},
    {1951,  6, 14, "UNIVAC I Delivered to US Census Bureau",
               "The first commercially available computer is delivered, processing data for the 1950 census."},
    {1957, 10,  4, "Sputnik 1 Launched",
               "The Soviet Union orbits the first artificial satellite, igniting the space race and accelerating US technology investment."},
    {1959,  7,  1, "Integrated Circuit Invented",       // approx
               "Jack Kilby and Robert Noyce independently create the integrated circuit, the foundation of all modern chips."},
    {1961, 11,  1, "CTSS Debuts at MIT",                // approx
               "The Compatible Time-Sharing System at MIT lets multiple users share a single mainframe simultaneously."},
    {1963,  6, 17, "ASCII Standard Published",
               "The American Standard Code for Information Interchange establishes a universal text encoding that persists to this day."},
    {1965,  4, 19, "Moore's Law Stated",
               "Gordon Moore predicts transistor counts will double roughly every two years — a benchmark that holds for decades."},
    {1969,  1,  1, "Unix Created at Bell Labs",         // approx
               "Ken Thompson and Dennis Ritchie begin developing Unix, the operating system that will underpin the internet."},
    {1969, 10, 29, "ARPANET First Message Sent",
               "The first network message is transmitted between UCLA and SRI; the system crashes after only 'LO'."},
    {1970,  1,  1, "Unix Epoch Begins",
               "January 1, 1970 becomes time zero for Unix systems — every computer in the Unix world counts from this moment."},
    {1971, 10,  1, "First Email Sent",                  // approx
               "Ray Tomlinson sends the first email between two machines and establishes the @ addressing convention."},
    {1971, 10, 25, "Phone Phreaking Era Begins",
               "John Draper discovers that a toy whistle can generate 2600 Hz and hijack AT&T's long-distance telephone network for free."},
    {1971, 11, 15, "Intel 4004 Microprocessor Released",
               "Intel ships the world's first commercially available microprocessor: a 4-bit chip clocked at 740 kHz."},
    {1972,  1,  1, "C Programming Language Created",    // approx
               "Dennis Ritchie creates C at Bell Labs; it will become one of the most influential programming languages ever written."},
    {1973,  5, 22, "Ethernet Invented at Xerox PARC",
               "Bob Metcalfe invents Ethernet, establishing the hardware foundation for local area networking."},
    {1975,  1,  1, "Altair 8800 Launches",
               "The Altair 8800 kit computer appears on the cover of Popular Electronics, igniting the personal computer revolution."},
    {1975,  4,  4, "Microsoft Founded",
               "Bill Gates and Paul Allen found Microsoft to write a BASIC interpreter for the Altair 8800."},
    {1976,  4,  1, "Apple Computer Founded",
               "Steve Jobs, Steve Wozniak, and Ronald Wayne found Apple Computer on April Fools' Day."},
    {1977,  6, 10, "Apple II Released",
               "The Apple II launches at the West Coast Computer Faire as one of the first mass-produced personal computers with colour graphics."},
    {1977,  8,  3, "TRS-80 and Commodore PET Launch",
               "Tandy and Commodore each ship a competing home computer, cementing 1977 as the year personal computing arrives."},
    {1978,  2, 16, "First Public BBS Goes Online",
               "The first public bulletin board system goes live in Chicago, connecting computer hobbyists via telephone modem."},
    {1979, 10, 17, "VisiCalc Released",
               "The first spreadsheet application launches for the Apple II, inventing a software category and justifying the PC for business."},
    {1981,  8, 12, "IBM PC Introduced",
               "IBM launches its Personal Computer, lending legitimacy to the industry and establishing an open architecture that others clone."},
    {1982,  8,  1, "Commodore 64 Released",             // approx
               "The C64 ships at $595 and goes on to become the best-selling single computer model of all time."},
    {1983,  1,  1, "Internet Officially Switches On",
               "ARPANET cuts over to TCP/IP on January 1, creating the operational backbone of the modern internet."},
    {1983,  6,  3, "WarGames Released",
               "The film WarGames introduces mainstream audiences to hacking culture and nearly starts a nuclear war on screen."},
    {1984,  1, 24, "Apple Macintosh Unveiled",
               "Apple introduces the Macintosh with a graphical interface in a landmark Super Bowl advertisement directed by Ridley Scott."},
    {1984,  7,  1, "Neuromancer Published",
               "William Gibson's debut novel coins the term 'cyberspace' and defines the cyberpunk aesthetic for a generation."},
    {1985, 11, 20, "Windows 1.0 Released",
               "Microsoft ships the first version of Windows as a graphical shell on top of MS-DOS."},
    {1986,  1,  8, "Hacker's Manifesto Published",
               "The Conscience of a Hacker appears in Phrack magazine hours after Loyd Blankenship is arrested by the FBI."},
    {1988, 11,  2, "Morris Worm Spreads Across the Internet",
               "The Morris Worm becomes the first widely publicised internet worm, infecting roughly 6,000 machines in one night."},
    {1989,  3, 12, "World Wide Web Proposed",
               "Tim Berners-Lee submits his proposal for a distributed information management system to his supervisors at CERN."},
    {1991,  8,  6, "WWW Goes Public",
               "The World Wide Web is opened to the public; the first website at CERN describes the project itself."},
    {1991,  9, 17, "Linux Kernel Released",
               "Linus Torvalds releases Linux 0.01, announcing it modestly as 'just a hobby, won't be big and professional.'"},
    {1993,  4, 22, "Mosaic Browser Released",
               "The first graphical web browser makes navigating the internet accessible to users without technical backgrounds."},
    {1993, 12, 10, "Doom Released by id Software",
               "id Software releases Doom, defining the first-person shooter genre and pioneering shareware software distribution."},
    {1994, 12, 15, "Netscape Navigator Released",
               "Netscape Navigator launches and drives explosive mainstream adoption of the World Wide Web."},
    {1995,  7, 16, "Amazon.com Opens",
               "Jeff Bezos launches Amazon as an online bookstore from his garage in Bellevue, Washington."},
    {1995,  8, 24, "Windows 95 Released",
               "Microsoft launches Windows 95 to massive fanfare, introducing the Start button, taskbar, and Plug and Play."},
    {1995, 12,  4, "JavaScript Created",
               "Brendan Eich creates JavaScript in ten days at Netscape; it will become the scripting language of the entire web."},
    {1997,  5, 11, "Deep Blue Defeats Kasparov",
               "IBM's Deep Blue defeats world chess champion Garry Kasparov in a landmark moment for artificial intelligence."},
    {1998,  9,  4, "Google Founded",
               "Larry Page and Sergey Brin incorporate Google Inc. after developing PageRank at Stanford University."},
    {1999,  6,  1, "Napster Launches",                  // approx
               "Napster enables peer-to-peer music file sharing and disrupts the entire recording industry within months."},
    {2000,  1,  1, "Y2K: No Apocalypse",
               "January 1, 2000 arrives without the widespread computer failures that had been feared — billions spent in preparation."},
    {2001,  1, 15, "Wikipedia Launches",
               "Jimmy Wales and Larry Sanger launch Wikipedia; it grows into the world's largest encyclopedia within a few years."},
    {2003,  4, 14, "Human Genome Project Completed",
               "Scientists announce the sequencing of the complete human genome, opening a new era in biology and medicine."},
    {2004,  2,  4, "Facebook Founded",
               "Mark Zuckerberg launches 'The Facebook' from his Harvard dorm room; it opens to the public two years later."},
    {2004, 11,  9, "Firefox 1.0 Released",
               "The Mozilla Foundation releases Firefox, launching a credible challenge to Internet Explorer's near-total dominance."},
    {2005,  2, 14, "YouTube Founded",
               "Three former PayPal employees found YouTube in a San Mateo garage; Google acquires it eighteen months later."},
    {2006,  3, 21, "Twitter Launches",
               "Jack Dorsey sends the first tweet — 'just setting up my twttr' — launching the microblogging platform."},
    {2007,  1,  9, "iPhone Announced",
               "Steve Jobs announces the iPhone, describing it as 'an iPod, a phone, and an internet communicator' rolled into one."},
    {2008, 10, 31, "Bitcoin Whitepaper Published",
               "Satoshi Nakamoto publishes 'Bitcoin: A Peer-to-Peer Electronic Cash System', proposing a decentralised digital currency."},
    {2009,  1,  3, "Bitcoin Genesis Block Mined",
               "The first Bitcoin block is mined with a newspaper headline embedded: 'Chancellor on brink of second bailout for banks.'"},
    {2010,  1, 27, "iPad Announced",
               "Apple announces the iPad, launching the tablet computing era and redefining portable media consumption."},
    {2013,  6,  6, "Snowden Reveals NSA Surveillance",
               "Edward Snowden leaks classified documents exposing the NSA's global mass surveillance programmes to The Guardian and Washington Post."},
    {2016,  3, 15, "AlphaGo Defeats World Go Champion",
               "Google DeepMind's AlphaGo defeats Lee Sedol at Go — a game long considered too complex and intuitive for AI."},
    {2020,  3, 11, "COVID-19 Pandemic Declared",
               "The WHO declares a global pandemic; remote work and digital infrastructure face an unprecedented overnight stress test."},
    {2022, 11, 30, "ChatGPT Launches",
               "OpenAI releases ChatGPT, making large language models accessible to the public and igniting a wave of AI investment."},
    {2038,  1, 19, "Year 2038 Problem Arrives",
               "32-bit Unix timestamps overflow; legacy systems that were never patched begin reporting impossible dates."},
};

/// @brief Total number of news events in the catalogue.
static const int NEWS_EVENT_COUNT =
    static_cast<int>(sizeof(ALL_EVENTS) / sizeof(ALL_EVENTS[0]));

// ── Helpers ───────────────────────────────────────────────────────────────────

/**
 * @brief Return whether the current in-game date has reached or passed an event.
 * @param ev  News event to evaluate.
 * @param dt  Current in-game date/time.
 * @return `true` when the event should trigger.
 */
static inline bool should_trigger_news_event(const NewsEvent& ev,
                                             const GameDateTime& dt) {
    if (dt.year != ev.year) return dt.year > ev.year;
    if (dt.month != ev.month) return dt.month > ev.month;
    return dt.day >= ev.day;
}
