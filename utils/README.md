# CSV to Notion

Simply export contract functions and variables to selected Notion page.

⚠WARNING: experimental, may subject to changes.

## How To Use

1. Create an integration at `developer.notion.com` and get an API key. Remember to share the project page to this integration.
2. Create a dedicated page for an audit project, and get its ID (an UUID-like string with optional `-`).
3. Need a stable proxy.

## Script

Under "utils/csv2notion" foler.

Linux:

```bash
npm install

export NOTION_KEY=secret_...
export PAGE_ID=<your page id>
npm run import
```

Windows:

```powershell
npm install

$env:NOTION_KEY = 'secret_...'
$env:PAGE_ID = '...'
Get-ChildItem -Path <path to csvs> -Filter *.csv -Recurse | %{$_.FullName} | %{node import.js $_}
```
