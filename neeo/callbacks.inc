CALLBACK(TestEvent) CODE({
    Component* Sender = (Component*)sender;
    printf("Sender %s Args: %d\r\n", Sender->GetID(), Args.size());
    for(unsigned int i=0; i<Args.size(); i++){
        printf("\tArg%u:%s: \r\n", i, Args[i].c_str());
    }
})

CALLBACK(HideHeader) CODE({
    if(DispMan->GetActiveScreen() && DispMan->GetActiveScreen()->GetHeader()){
        DispMan->GetActiveScreen()->GetHeader()->Hide();
    }
})

CALLBACK(ShowHeader) CODE({
    if(DispMan->GetActiveScreen() && DispMan->GetActiveScreen()->GetHeader()){
        DispMan->GetActiveScreen()->GetHeader()->Show();
    }
})

CALLBACK(HideTopPanel) CODE({
    if(DispMan->GetTopPanel()){
        DispMan->GetTopPanel()->Hide();
    }
})

CALLBACK(ClearList) CODE({
    ((ListView*)DispMan->GetScreen("DynamicList"))->ClearList();
})

CALLBACK(LoadList) CODE({
    if(Args[0] == "1"){
        string Data((const char*)list1.data, list1.size);
        ((ListView*)DispMan->GetScreen("DynamicList"))->AddToList(DispMan, Data);
    } else {
        string Data((const char*)list2.data, list2.size);
        ((ListView*)DispMan->GetScreen("DynamicList"))->AddToList(DispMan, Data);
    }
})

CALLBACK(ShowTopPanel) CODE({
    if(DispMan->GetTopPanel()){
        DispMan->GetTopPanel()->Show();
    }
})

CALLBACK(ScrollCallback) CODE({
    ScrollBar* Sender = (ScrollBar*)sender;
    printf("ScrollBar: %s    New value: %d\r\n", Sender->GetID(), (int)Sender->GetValue());
})

CALLBACK(UpdateJnFirmware) CODE({
    Component* Sender = (Component*)sender;
    uint8_t* jn_firmware;
    uint32_t jn_firmware_size = 0;

    printf("Sender %s Args: %d\r\n", Sender->GetID(), Args.size());

    printf(">>>>CC3100 init\r\n");
    int retVal = 0;
    if(wifiInit == 0)
    {
        retVal = wifi_init(conf.ssid, conf.ssid_length, conf.pass, conf.pass_length);
        wifiInit = 1;
    }
    if(retVal < 0){
        printf("Failed to init WIFI");
    } else {
        if(wifi_download(Args[0].c_str(), (const char*)conf.host_ip, &jn_firmware_size, &jn_firmware) == 0)
        {
            UploadFirmwareToJn(jn_firmware, jn_firmware_size);
        }
        else
            printf("wifi_download failed!\n\r");
    }
})

/*
    JN_get_request(uri)
*/
CALLBACK(JN_get_request) CODE({
    const uint16_t responseBufferLen = 512;
    uint8_t response[responseBufferLen];
    int actualLength;

    actualLength = JN_rest_request(REST_GET, Args[0], "", response, responseBufferLen);

    if(actualLength <= 0) {
        log_msg(LOG_ERROR, __cfunc__, "GET request failed!");
        return;
    }

    log_msg(LOG_INFO, __cfunc__, "GET response: ");
    for(int i =0; i < actualLength; i++)
        printf("%c", response[i]);
    printf("\r\n");
})

/*
    JN_post_request(uri, data)
*/
CALLBACK(JN_post_request) CODE({
    const uint8_t responseBufferLen = 128;
    uint8_t response[responseBufferLen];
    int actualLength;

    actualLength = JN_rest_request(REST_POST, Args[0], Args[1], response, responseBufferLen);

    if(actualLength <= 0) {
        log_msg(LOG_ERROR, __cfunc__, "POST request failed!");
        return;
    }

    log_msg(LOG_INFO, __cfunc__, "POST response: ");
    for(int i =0; i < actualLength; i++)
        printf("%c", response[i]);
    printf("\r\n");
})

/*
    JN_put_request(uri, data)
*/
CALLBACK(JN_put_request) CODE({
    const uint8_t responseBufferLen = 128;
    uint8_t response[responseBufferLen];
    int actualLength;

    actualLength = JN_rest_request(REST_PUT, Args[0], Args[1], response, responseBufferLen);

    if(actualLength <= 0)
    {
        log_msg(LOG_ERROR, __cfunc__, "PUT request failed!");
        return;
    }

    log_msg(LOG_INFO, __cfunc__, "PUT response: ");
    for(int i =0; i < actualLength; i++)
        printf("%c", response[i]);
    printf("\r\n");
})

/*
    JN_delete_request(uri)
*/
CALLBACK(JN_delete_request) CODE({
    const uint8_t responseBufferLen = 128;
    uint8_t response[responseBufferLen];
    int actualLength;

    actualLength = JN_rest_request(REST_DELETE, Args[0], "", response, responseBufferLen);

    if(actualLength <= 0)
    {
        log_msg(LOG_ERROR, __cfunc__, "DELETE request failed!");
        return;
    }

    log_msg(LOG_INFO, __cfunc__, "DELETE response: ");
    for(int i =0; i < actualLength; i++)
        printf("%c", response[i]);
    printf("\r\n");
})

CALLBACK(UpdateJnFirmwareLocal) CODE({
    long jn_firmware_size;
    log_msg(LOG_INFO, __cfunc__, "Openning firmware file");
    uint8_t * jn_firmware = romfs_allocate_and_get_file_content(Args[0].c_str(), false, &jn_firmware_size);
    log_msg(LOG_INFO, __cfunc__, "Upgrading JN5168 firmware");
    UploadFirmwareToJn(jn_firmware, jn_firmware_size);
    log_msg(LOG_INFO, __cfunc__, "Done!");
    custom_free(jn_firmware);
})

CALLBACK(Beep) CODE({
    if(Args.size() % 3 != 0) return;
    int beeps = Args.size() / 3;

    for(int j = 0; j<beeps*3; j+=3){
        PWM_BUZZER_Beep(atoi(Args[j].c_str()), atoi(Args[j+1].c_str()));
        msleep(atoi(Args[j+2].c_str()));
    }
})

CALLBACK(VolumeUp) CODE({
    int actualProgress = ((ProgressBar*)DispMan->GetPopupFromContainer("volume")->GetElement("progress"))->GetProgressValue();
    ((ProgressBar*)DispMan->GetPopupFromContainer("volume")->GetElement("progress"))->SetProgressValue(actualProgress + atoi(Args[0].c_str()));
    DispMan->ShowPopup("volume", 1500);
})

CALLBACK(VolumeDown) CODE({
    int actualProgress = ((ProgressBar*)DispMan->GetPopupFromContainer("volume")->GetElement("progress"))->GetProgressValue();
    ((ProgressBar*)DispMan->GetPopupFromContainer("volume")->GetElement("progress"))->SetProgressValue(actualProgress - atoi(Args[0].c_str()));
    DispMan->ShowPopup("volume", 1500);
})

CALLBACK(ChangeScreen) CODE({
    int screen_animation = 0;
    if(Args.size() > 1) screen_animation = atoi(Args[1].c_str());
    else screen_animation = 0;
    DispMan->SetActiveScreen(Args[0].c_str(), screen_animation);
})

CALLBACK(Pair) CODE({
    log_msg(LOG_INFO, __cfunc__, "Pairing with: %s", Args[0].c_str());
    log_msg(LOG_INFO, __cfunc__, "Spawning Pair thread");
    pthread_create(&JNThreadHandle, &JNThreadAttr, &JNThread, (void*)&Args[0]);
    set_thread_prio(JNThreadAttr, JNThreadHandle, JN_THREAD_PRIO);
})

CALLBACK(FallBack) CODE({

    int res = BuildGUI(&GlobalXml);
    offline_mode = true;

    if(res == -1){
        //Todo Handle error
    } else {
        DispMan->SetActiveScreen("0", 0);
    }
})

CALLBACK(JN_init_pairing_button) CODE({
    vector<string> cp6list;
    if (JN_get_cp6_list(&cp6list) == 0)
    {
        ((ListView*)DispMan->GetScreen("CP6List"))->ClearList();
        for(unsigned int i=0; i<cp6list.size(); i++)
        {
            log_msg(LOG_INFO, __cfunc__, "CP-6 at: %s", cp6list[i].c_str());
            ListItem* TempListItem;
            TempListItem = new ListItem();
            TempListItem->SetBackgroundColor(COLOR_ARGB8888_WHITE);
            TempListItem->SetForegroundColor(COLOR_ARGB8888_DARKGRAY);
            TempListItem->SetDescriptionColor(COLOR_ARGB8888_DARKBLUE);
            TempListItem->SetType(ListItem::StdListField);
            TempListItem->SetSize(((ListView*)DispMan->GetScreen("CP6List"))->GetWidth(), 110);
            TempListItem->SetTextFont(DispMan->TryGetFontPtr("large"));
            TempListItem->SetText(cp6list[i].c_str());
            Event::ArgVector arg;
            arg.push_back(cp6list[i].c_str());
            Event ev = Event(DispMan->GetCallbackFromContainer("Pair"), arg);
            TempListItem->SetOnClickEvent(ev);
            ((ListView*)DispMan->GetScreen("CP6List"))->AddElement(TempListItem);

        }
        DispMan->SetActiveScreen("CP6List", 0);
    }
    else
    {
        log_msg(LOG_ERROR, __cfunc__, "Failed to get CP-6 list");
    }
})

CALLBACK(JN_get_zero_config_button) CODE({
    zero_config_t conf;

    if(JN_get_zero_config(Args[0].c_str(), &conf))
    {
        log_msg(LOG_ERROR, __cfunc__, "Failed to receive credentials.");
    }
    else
    {
        log_msg(LOG_INFO, __cfunc__, "SSID: %s, password: %s. GUI file at %s.", conf.ssid, conf.pass, conf.gui_loc);
    }
})


CALLBACK(ShowPopup) CODE({
    DispMan->ShowPopup("hello", Args[0].c_str(), 1000);
})

CALLBACK(JNCheck) CODE({
    if(strcmp(Args[0].c_str(), "yes") == 0){
        firmwareCheckVal = 1;
    } else {
        firmwareCheckVal = 2;
    }
    DispMan->ClosePopup();
})

CALLBACK(JNPairingCancel) CODE({
    pairingCancel = true;
    log_msg(LOG_INFO, __cfunc__, "Pairing cancel!");
})

CALLBACK(HidePopup) CODE({
    DispMan->ClosePopup();
})

CALLBACK(checkFirmware) CODE({
    pthread_create(&FirmwareUpdatesThreadHandle, &FirmwareUpdatesThreadAttr, &FirmwareUpdatesThread, NULL);
    set_thread_prio(FirmwareUpdatesThreadAttr, (pthread_t)FirmwareUpdatesThread, FIRMWAREUPDATE_THREAD_PRIO);
})

CALLBACK(GetLocalIP) CODE({
    char local_ip[128];
    uint16_t local_ip_len = 0;
    if(JN_get_local_ip(local_ip, 128, &local_ip_len) < 0)
        return;
    else
        log_msg(LOG_INFO, __cfunc__, "Local IPv6 address: %s", local_ip);
})
