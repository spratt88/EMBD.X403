// proxy for GET

export async function get({ params }) {
    console.log(params);
    const proxyUrl = `http://esp32s3_7650.local/api/${params.route}`;
    const response = await fetch(proxyUrl);
    const body = await response.json();
    return { body };
   }

// same for POST

export async function post({ params, request }) {
    const bodyIncoming = await request.json();
    console.log("post hit", params, bodyIncoming);
    const proxyUrl = `http://esp32s3_7650.local/api/${params.route}`;
    const response = await fetch(proxyUrl, { method: "POST", body: JSON.stringify(bodyIncoming) });
    if (response.status != 200) return { status: response.status };
    const body = await response.json();
    return { body };
  }
  