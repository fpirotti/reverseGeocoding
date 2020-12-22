decode.bing<-function(content){
  if(content$statusCode!=200){
    return(NULL)
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
  latlng<-unlist(content$results[[1]]$geometry$location)
  accur<-content$results[[1]]$geometry$location_type
  formattedAddress<-content$results[[1]]$formatted_address
  
  list(lat=latlng[[1]],
       long=latlng[[2]],
       accur=accur,
       formattedAddress=formattedAddress, type="G")
}

decode.here<-function(content){
  if(!is.null(content$error)){
    return(content$error_description)
  }

  
  latlng<-unlist(content$items[[1]]$position)
  accur<-content$items[[1]]$resultType
  formattedAddress<-content$items[[1]]$address$label
  
  list(lat=latlng[[1]],
       long=latlng[[2]],
       accur=accur,
       formattedAddress=formattedAddress, type="H")
}



parseContent<-function(content, type){
  
  if(type=="bing"){
    dd<-decode.bing(content)
  } 
  else if(type=="google"){
    dd<-decode.google(content)
  }
  else if(type=="here"){
    dd<-decode.here(content)
  } else {
    return(NULL)
  }
  
  dd
}