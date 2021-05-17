# ovreallGcalls<-list()
# ovreallGcalls[[format(Sys.time(), "%Y%m")]]<-0
# saveRDS(ovreallGcalls,"ovreallGcalls.rds")

if(file.exists("cacheFile.rds")) {
  cacheFile<-readRDS("cacheFile.rds")
} else {
  cacheFile<-list(bing=list(), google=list(), here=list())
}

ovreallGcalls<-readRDS("ovreallGcalls.rds")

keys <- list(
  G = c(
    "AIzaSyDvfSqURa60jKdXBEcL7hnwsvwEWhYgkm4",
    "AIzaSyDeAaUewDUcIriQsqOX1NNGxatX8WkbZIE",
    "AIzaSyCMV7dLRGHwkTyuOhojM03Sgh5Y3IZAGXM",
    "AIzaSyCpQoUQ9Pnz3jzCE8KpRWW1bBuzT-_a97M"
  ),
  
  B = c(
    "AqfXHrbz_Qm4PuVk7lAPMA7ue1vuhMbJgLI-3ZwGBQ3QRIqpMcMBDx2N6OZdJYoj",
    "At2br5wQAqbVaQW6Fn9eJTTbnrMiaSkriO4lx6YdfDSGNsGI3lMYMYYjlmCbjU-5",
    "AtFnmH3kI3H_eOuB1eyxEd66fISOnuj6R7nuM_uhPAhtB7_yL1bTr0Wt4gj_ZVf8",
    "AjvjPYuoA4IgNeooKvodDLcxbVL1F8RdIxXUeYsb6PgiVapURz_PbbWvOxVKmNps"
  ),
  
  H = c("YeoIkazjfVa2WND4PUvLAjDR7VGRV_QZnfQ80TjTZSc")
)
API.list <-
  list(
    G = "https://maps.googleapis.com/maps/api/geocode/json?key=%s&region=IT&address=%s",
    B = "http://dev.virtualearth.net/REST/v1/Locations?CountryRegion=IT&key=%s&addressLine=%s",
    B2 = "https://www.bing.com/maps/overlay?q=%s",
    H = "https://geocode.search.hereapi.com/v1/geocode?limit=1&in=countryCode:ITA&apiKey=%s&q=%s"
  )




readURL<-function(url){

  r <- GET((url))
  if(status_code(r)!=200){
    shinyWidgets::show_alert(sprintf("Errore. Codice %s:    ---  %s",
                                     status_code(r), http_status(r)$message,
                                     url )) 
    return(NULL)
  }
  
  content(r, "parsed")
}

decode.functions<-list()


decode.here<-function(content){
  if(!is.null(content$error)){
    return(content$error_description)
  }
  
  if(length(content$items)==0){
    return( list(lat=0,
                 long=0,  
                 accur="NA",
                 formattedAddress="NA", 
                 type="H") )   
  }
  
  latlng<-unlist(content$items[[1]]$position)
  accur<-content$items[[1]]$resultType
  formattedAddress<-content$items[[1]]$address$label
  
  list(lat=latlng[[1]],
       long=latlng[[2]],
       accur=accur,
       formattedAddress=formattedAddress, 
       type="H")
}


decode.bing<-function(content){
  
  if(content$statusCode!=200){ 
    return(as.character(content$statusDescription))
  }
  
  if(length(content$resourceSets)==0){
    return( list(lat=NA,
                 long=NA,  accur=NA,
                 formattedAddress=NA, type="B") )   
  }
  
  
  if(length(content$resourceSets[[1]]$resources)==0){
    return( list(lat=NA,
                 long=NA,  accur=NA,
                 formattedAddress=NA, type="B") )   
  }
  
  latlng<-unlist(content$resourceSets[[1]]$resources[[1]]$point$coordinates)
  accur<-content$resourceSets[[1]]$resources[[1]]$confidence
  formattedAddress<-content$resourceSets[[1]]$resources[[1]]$address$formattedAddress
  
  list(lat=latlng[[1]],
       long=latlng[[2]],
       accur=accur,
       formattedAddress=formattedAddress,
       type="B")
}


decode.google<-function(content){
  if(content$status!="OK"){
    return(content$status)
  }
  
  if(length(content$results)==0){
    return( list(lat=0,
                 long=0,  accur="NA",
                 formattedAddress="NA", type="G") )   
  }
  
  latlng<-unlist(content$results[[1]]$geometry$location)
  accur<-content$results[[1]]$geometry$location_type
  formattedAddress<-content$results[[1]]$formatted_address
  
  list(lat=latlng[[1]],
       long=latlng[[2]],
       accur=accur,
       formattedAddress=formattedAddress, type="G")
}


decode.functions[["B"]]<-decode.bing
decode.functions[["H"]]<-decode.here
decode.functions[["G"]]<-decode.google
  
getContent<-function(engine, address){
  url<-sprintf(API.list[[engine]], 
               keys[[engine]][[1]],
               URLencode(address) )
  readURL(url) 
}

parseContent<-function( type, address){
  
  if(type=="ALL"){
    for(f in names(decode.functions)){
      
      if(f=="G"){
        #### verifica che google non sia troppo sfruttato ------
        tt<-format(Sys.time(), "%Y%m")
        if(is.null(ovreallGcalls[[format(Sys.time(), "%Y%m")]])) {
          ovreallGcalls[[tt]]<<-0
        } else {
          ovreallGcalls[[tt]]<<- ovreallGcalls[[tt]]+1
        } 
        print(paste("Arrivato a Google:", content, " N.", ovreallGcalls[[tt]]))
        if(ovreallGcalls[[tt]]>100){
          
          shinyWidgets::show_alert( "Troppo Google questo mese") 
          return(NULL)
        }
      }
      
      content<-getContent(f, address)
      
      if(is.null(content)){
        next
      }
      
      dd<-decode.functions[[f]](content)
      
      if(is.list(dd)){
        cacheFile[[dd$type]][[address]]<<-dd
        if(length(cacheFile[[dd$type]])%%100==0) {
          print("Saving cache")
          saveRDS(cacheFile, "cacheFile.rds")
        }
      }
      if(is.list(dd) &&  shiny::isTruthy(dd$lat) ){
        #cacheFile[[dd$type]][[address]]<<-dd
        break
      } 
    }
  } else {
    content<-getContent(type, address)
    dd<-decode.functions[[type]](content)
    if(is.list(dd)){
      cacheFile[[dd$type]][[address]]<<-dd
      if(length(cacheFile[[dd$type]])%%100==0) {
        print("Saving cache")
        saveRDS(cacheFile, "cacheFile.rds")
      }
    }
  }
  
  dd
  
}