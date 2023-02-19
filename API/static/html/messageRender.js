let cookies = document.cookie;

// find messages
if(cookies && cookies !== "") {
    let cookieArray = cookies.split("; ");
    for(let i = 0; i < cookieArray.length; i++) {
        let cookie = cookieArray[i];
        if(cookie.startsWith("message=")) {
            // set alert
            alert(cookie.substring(8));
            // delete cookie
            document.cookie = "message=; expires=Thu, 01 Jan 1970 00:00:00 UTC;";
        }
    }
}

