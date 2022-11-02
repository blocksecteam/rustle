"use strict";

const { program } = require('commander');
program.version('0.0.1');

const { Client } = require('@notionhq/client');
const fs = require('fs');

// var HttpsProxyAgent = require('https-proxy-agent');
// var proxy = process.env.https_proxy || process.env.http_proxy;
var config = { auth: process.env.NOTION_KEY };
// if (typeof proxy !== 'undefined') {
//     console.log(`using proxy: ${proxy}`);
//     config.agent = new HttpsProxyAgent(proxy);
// }
const notion = new Client(config);

function parseCSV(filepath) {
    let content = '';
    try {
        content = fs.readFileSync(filepath).toString('utf-8');
    } catch (err) {
        console.error(`\nError found while reading the following file: ${filepath}\n`);
        throw err;
    }

    var allTextLines = content.split(/\r\n|\n/);
    var headers = allTextLines[0].split(',');
    var entries = [];
    for (let i = 1; i < allTextLines.length - 1; i++) {
        let obj = {}
        let str = allTextLines[i]
        let s = ''

        let flag = 0
        for (let j = 0; j < str.length; j++) {
            let skip = false
            let ch = str[j]
            if (ch === '"') {
                if (j !== 0 && str[j - 1] === '\\') {
                }
                else {
                    skip = true

                    if (flag === 0)
                        flag = 1
                    else if (flag == 1)
                        flag = 0
                }
            }
            if (ch === ',' && flag === 0) ch = '@'
            if (!skip)
                s += ch
            s = s.replace('\\', '')
        }

        let properties = s.split("@")

        console.log(properties);

        for (let j in headers) {
            obj[headers[j].replace(/^['|"](.*)['|"]$/, "$1").trim()] = properties[j].replace(/^['|"](.*)['|"]$/, "$1").trim();
        }

        entries.push(obj)
    }

    return {
        name: filepath.split("/").pop().split("\\").pop().replace('.csv', ''),
        entries: entries,
    };
}

async function createEntry(table_id, e) {

    var table = {
        "public(near_bindgen)": "❗️PUBLIC",
        "public": "❗️PUBLIC",
        "default": "❗️PUBLIC",
        "external": "❗️EXTERNAL",
        "private(near_bindgen)": "🔐PRIVATE",
        "private": "🔐PRIVATE",
        "internal(near_bindgen)": "🔒INTERNAL",
        "internal": "🔒INTERNAL"
    };

    const response = await notion.pages.create({
        parent: {
            database_id: table_id
        },

        "properties": {
            "Name": {
                "title": [
                    {
                        "text": {
                            "content": e.name,
                        },
                    },
                ],
            },
            "Visibility": {
                "select": {
                    "name": table[e.visibility]
                }
            },
            "Type": {
                "rich_text": [
                    {
                        "text": {
                            "content": e.type,
                        },
                    },
                ],
            },
            "Macro": {
                "rich_text": [
                    {
                        "text": {
                            "content": e.macro,
                        },
                    },
                ],
            },
            "Modifiers": {
                "rich_text": [
                    {
                        "text": {
                            "content": e.modifier,
                        },
                    },
                ],
            },
            "Status": {
                "select": {
                    "name": "🚧WORKING"
                }
            },
            "High": {
                "rich_text": [
                    {
                        "text": {
                            "content": e.high,
                        },
                    },
                ],
            },
            "Medium": {
                "rich_text": [
                    {
                        "text": {
                            "content": e.medium,
                        },
                    },
                ],
            },
            "Low": {
                "rich_text": [
                    {
                        "text": {
                            "content": e.low,
                        },
                    },
                ],
            },
            "Info": {
                "rich_text": [
                    {
                        "text": {
                            "content": e.info,
                        },
                    },
                ],
            },
        }
    });
    return response;
}

async function createTable(page_id, title) {
    const response = notion.databases.create({
        "parent": {
            "page_id": page_id,
        },
        "title": [
            {
                "text": {
                    "content": title,
                }
            }
        ],
        "properties": {
            "Name": {
                "title": {}
            },
            "Visibility": {
                "select": {
                    "options": [
                        {
                            "name": "❗️PUBLIC",
                            "color": "red"
                        },
                        {
                            "name": "❗️EXTERNAL",
                            "color": "yellow"
                        },
                        {
                            "name": "🔐PRIVATE",
                            "color": "blue"
                        },
                        {
                            "name": "🔒INTERNAL",
                            "color": "green"
                        }
                    ]
                }
            },
            "Type": {
                "rich_text": {}
            },
            "Macro": {
                "rich_text": {}
            },
            "Modifiers": {
                "rich_text": {}
            },
            "Description": {
                "rich_text": {}
            },
            "Status": {
                "select": {
                    "options": [
                        {
                            "name": "🚧WORKING",
                            "color": "yellow"
                        },
                        {
                            "name": "❓QUESTIONABLE",
                            "color": "red"
                        },
                        {
                            "name": "✔️DONE",
                            "color": "green"
                        },
                        {
                            "name": "❗ISSUE",
                            "color": "red"
                        }
                    ]
                }
            },
            "High": {
                "rich_text": {}
            },
            "Medium": {
                "rich_text": {}
            },
            "Low": {
                "rich_text": {}
            },
            "Info": {
                "rich_text": {}
            },

        }
    });
    return (await response).id;
}

async function createTableForData(page_id, contract_name, entries) {
    const delay = 500;

    var table_id = await createTable(page_id, contract_name);
    console.log(`table_id: ${table_id}`)

    entries = entries.reverse();
    console.log("---");
    for (let index = 0; index < entries.length; ++index) {
        const e = entries[index];
        await createEntry(table_id, e);
        await new Promise(resolve => setTimeout(resolve, delay));
        console.log(e.name);
    }
    return table_id;
}

async function createEntry_Summary(table_id, e) {

    const response = await notion.pages.create({
        parent: {
            database_id: table_id
        },

        "properties": {
            "File": {
                "title": [
                    {
                        "text": {
                            "content": e.file,
                        },
                    },
                ],
            },
            "Name": {
                "rich_text": [
                    {
                        "text": {
                            "content": e.name,
                        },
                    },
                ],
            },
            "High": {
                "rich_text": [
                    {
                        "text": {
                            "content": e.high,
                        },
                    },
                ],
            },
            "Medium": {
                "rich_text": [
                    {
                        "text": {
                            "content": e.medium,
                        },
                    },
                ],
            },
            "Low": {
                "rich_text": [
                    {
                        "text": {
                            "content": e.low,
                        },
                    },
                ],
            },
            "Info": {
                "rich_text": [
                    {
                        "text": {
                            "content": e.info,
                        },
                    },
                ],
            },
        }
    });
    return response;
}

async function createTable_Summary(page_id, title) {
    const response = notion.databases.create({
        "parent": {
            "page_id": page_id,
        },
        "title": [
            {
                "text": {
                    "content": title,
                }
            }
        ],
        "properties": {
            "File": {
                "title": {}
            },
            "Name": {
                "rich_text": {}
            },
            "High": {
                "rich_text": {}
            },
            "Medium": {
                "rich_text": {}
            },
            "Low": {
                "rich_text": {}
            },
            "Info": {
                "rich_text": {}
            },

        }
    });
    return (await response).id;
}

async function createTableForData_Summary(page_id, contract_name, entries) {
    const delay = 500;

    var table_id = await createTable_Summary(page_id, contract_name);
    console.log(`table_id: ${table_id}`)

    entries = entries.reverse();
    console.log("---");
    for (let index = 0; index < entries.length; ++index) {
        const e = entries[index];
        await createEntry_Summary(table_id, e);
        await new Promise(resolve => setTimeout(resolve, delay));
        console.log(e.name);
    }
    return table_id;
}

(async () => {
    if (process.env.NOTION_KEY == undefined) {
        console.log("no NOTION_KEY provided\n");
        return;
    }

    let page_id = process.env.PAGE_ID;
    if (page_id === undefined) {
        console.log("no page_id provided\n");
        return;
    }

    console.log(`root page_id: ${page_id}`)

    const response = await notion.pages.create({
        parent: {
            page_id: page_id,
        },
        properties: {}

    });
    page_id = (response).id;

    console.log(`project page_id: ${page_id}`)

    for (let i = 2; i < process.argv.length; ++i) {
        console.log(`process.argv[i]: ${process.argv[i]}`)
        let data = parseCSV(process.argv[i]);
        console.log(`table_name: ${data.name}`)
        let table_id;
        if (process.argv[i].endsWith("summary.csv")) {
            table_id = await createTableForData_Summary(page_id, data.name, data.entries);
        } else {
            table_id = await createTableForData(page_id, data.name, data.entries);
        }
        console.log(`table_id processed: ${table_id}\n`)
    }
})()
